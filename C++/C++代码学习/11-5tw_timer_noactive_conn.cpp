// 这个示例程序展示了“非活跃连接踢出”机制：
// 1. 主线程使用 epoll 处理所有 I/O 事件。
// 2. 通过 alarm 定时发出 SIGALRM 信号，提醒我们检查超时连接。
// 3. 信号在异步上下文里被写入管道，回到 epoll 主循环统一处理。
// 4. 每个客户端连接都挂在一个定时器上，超时后会自动关闭。
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <libgen.h>
#include "11-5tw_timer.h"
#define FD_LIMIT 65535        // 我们能同时处理的最大连接数（为了开辟 users 数组）
#define MAX_EVENT_NUMBER 1024 // epoll_wait 一次最多返回的事件数量
#define TIMESLOT 5            // 定时器基础间隔：每 5 秒触发一次 SIGALRM

static int pipefd[2]; // 全局管道：sig_handler 写入，主线程在 epoll 中读取
// static sort_timer_lst timer_lst; // 封装好的升序定时器链表，实现“超时踢出”
static time_wheel timer_tw; // 时间轮的槽数组
static int epollfd = 0;     // 主循环使用的 epoll 实例句柄，定时器回调也需要访问

// 将现实时间（秒）转换为时间轮需要的“tick”数量，保证至少为 1
int to_wheel_ticks(int timeout_seconds)
{
    if (timeout_seconds <= 0)
    {
        return 1;
    }
    int ticks = timeout_seconds / TIMESLOT;
    if (timeout_seconds % TIMESLOT)
    {
        ++ticks;
    }
    return ticks < 1 ? 1 : ticks;
}

// 将某个文件描述符设置为“非阻塞”模式，避免 read/write 阻塞住整个事件循环
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 把一个新的文件描述符加入 epoll 监听，使用“边沿触发 + 可读事件”
void addfd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}
// 信号处理函数：在异步上下文里“轻量化”地记录信号
void sig_handler(int sig)
{
    int save_errno = errno;              // 第1步：保存errno
    int msg = sig;                       // 第2步：将信号编号存入msg
    send(pipefd[1], (char *)&msg, 1, 0); // 第3步：写入管道
    errno = save_errno;                  // 第4步：恢复errno
}

// 注册一个信号，把处理函数设置成 sig_handler
void addsig(int sig)
{
    struct sigaction sa;                     // 定义信号动作结构体
    memset(&sa, '\0', sizeof(sa));           // 清空结构体
    sa.sa_handler = sig_handler;             // 设置处理函数
    sa.sa_flags |= SA_RESTART;               // 设置标志位
    sigfillset(&sa.sa_mask);                 // 阻塞所有信号
    assert(sigaction(sig, &sa, NULL) != -1); // 注册信号处理
}

// 有 SIGALRM 信号时调用：推进定时器链表，并重新设定下一次闹钟
void timer_handler()
{
    timer_tw.tick(); // Move the timer list forward and run expired callbacks
    alarm(TIMESLOT); // Rearm the alarm so it fires again after TIMESLOT seconds
}

// 定时器回调：真正执行“踢掉”操作
void cb_func(client_data *user_data)
{
    // 从epoll_ctl中删除该文件描述符
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    // 关闭socket文件描述符
    close(user_data->sockfd);
    printf("[timeout] close idle connection fd=%d\n", user_data->sockfd);
    user_data->timer = NULL; // 清除悬挂的定时器指针
}

// 客户端有数据活动时，删除旧定时器并重新创建一个新的倒计时
void refresh_timer(client_data *user_data)
{
    if (!user_data || !user_data->timer)
    {
        return;
    }

    tw_timer *old_timer = user_data->timer;
    void (*cb)(client_data *) = old_timer->cb_func;
    client_data *bound_user = old_timer->user_data;

    timer_tw.del_timer(old_timer); // 删除旧的时间轮节点（内部会释放内存）

    tw_timer *new_timer = timer_tw.add_timer(to_wheel_ticks(3 * TIMESLOT));
    if (!new_timer)
    {
        printf("[timer] refresh failed for fd=%d\n", bound_user ? bound_user->sockfd : -1);
        user_data->timer = NULL;
        return;
    }

    new_timer->cb_func = cb;
    new_timer->user_data = bound_user;
    user_data->timer = new_timer;
    printf("[timer] refreshed connection fd=%d\n", bound_user ? bound_user->sockfd : -1);
}

int main(int argc, char *argv[])
{
    // 新手提示：程序需要在终端中以“./11-5tw_timer_noactive_conn <IP> <PORT>”的形式运行
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;              // 使用 IPv4
    inet_pton(AF_INET, ip, &address.sin_addr); // 将字符串形式的 IP 转为二进制格式
    address.sin_port = htons(port);            // 主机字节序转网络字节序

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 将 socket 绑定到指定 IP:PORT 上
    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5); // 创建 epoll 实例（参数 >0 即可）
    assert(epollfd != -1);
    addfd(epollfd, listenfd); // 监听 socket 也要放进 epoll，随时处理新连接
    // 步骤1：创建一对相互连接的套接字（全双工管道）
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    // 现在有 pipefd[0] 和 pipefd[1] 两个描述符
    // 写入 pipefd[1] 的数据可以从 pipefd[0] 读取
    // 写入 pipefd[0] 的数据可以从 pipefd[1] 读取

    // 步骤2：将写端设置为非阻塞
    setnonblocking(pipefd[1]);
    // 这样信号处理函数中的 send 不会阻塞

    // 步骤3：将读端加入epoll监控
    addfd(epollfd, pipefd[0]);
    // epoll现在会监控 pipefd[0] 的"可读"事件
    // 当有数据到达时，epoll_wait 会返回

    // 将需要关注的信号都添加上
    addsig(SIGALRM);
    addsig(SIGTERM);
    bool stop_server = false;

    client_data *users = new client_data[FD_LIMIT]; // 预分配数组，索引就是文件描述符
    bool timeout = false;                           // 标记：是否收到了需要处理的 SIGALRM
    alarm(TIMESLOT);                                // 启动第一次定时器，5 秒后触发 SIGALRM

    while (!stop_server)
    {
        // epoll_wait 阻塞等待“感兴趣的文件描述符”发生事件
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                addfd(epollfd, connfd);
                users[connfd].address = client_address;
                users[connfd].sockfd = connfd;
                // 为新连接分配专属定时器。当定时器被触发时，就会调用 cb_func 关闭socket
                // tw_timer *timer = new tw_timer();
                // timer->user_data = &users[connfd];
                // timer->cb_func = cb_func;
                // time_t cur = time(NULL);
                // timer->expire = cur + 3 * TIMESLOT;
                // users[connfd].timer = timer;
                // timer_tw.add_timer(timer);
                tw_timer *timer = timer_tw.add_timer(to_wheel_ticks(3 * TIMESLOT));
                timer->user_data = &users[connfd];
                timer->cb_func = cb_func;
                // 建立双向关联
                users[connfd].timer = timer;
            }
            else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN))
            {
                // 检查当前的文件描述符是否是管道的读取端且有可读事件
                // pipefd[0] 是管道的读取端，events[i].events & EPOLLIN 表示可读事件
                int sig;
                char signals[1024];                                 // 用来存储从管道中读取的信号数据，一个信号一个字节，所以后续就可以直接循环ret来
                ret = recv(pipefd[0], signals, sizeof(signals), 0); // 从管道读取数据

                if (ret == -1)
                {
                    // 如果读取失败（可能是信号中断等情况），忽略这次事件并继续处理其他事件
                    continue;
                }
                else if (ret == 0)
                {
                    // 如果返回值为0，说明管道已关闭，继续处理下一个事件
                    continue;
                }
                else
                {
                    // 如果读取成功，处理读取到的信号
                    for (int i = 0; i < ret; ++i)
                    {
                        switch (signals[i])
                        {
                        case SIGALRM:
                        {
                            // 如果收到 SIGALRM 信号，表示定时器超时
                            // 设置 timeout 标志，表示发生了超时事件
                            timeout = true;
                            break;
                        }
                        case SIGTERM:
                        {
                            // 如果收到 SIGTERM 信号，表示请求终止服务器
                            // 设置 stop_server 标志，表示需要停止服务器
                            stop_server = true;
                            break;
                        }
                        }
                    }
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                memset(users[sockfd].buf, '\0', BUFFER_SIZE);
                ret = recv(sockfd, users[sockfd].buf, BUFFER_SIZE - 1, 0);
                printf("[io] recv %d bytes from fd=%d: %s\n", ret, sockfd, users[sockfd].buf);
                tw_timer *timer = users[sockfd].timer;
                if (ret < 0)
                {
                    // 读取失败：如果不是“暂时无数据”，说明连接出问题了，直接关闭
                    if (errno != EAGAIN)
                    {
                        cb_func(&users[sockfd]);
                        if (timer)
                        {
                            timer_tw.del_timer(timer);
                            users[sockfd].timer = NULL;
                        }
                    }
                }
                else if (ret == 0)
                {
                    // ret == 0 表示客户端主动关闭连接
                    cb_func(&users[sockfd]);
                    if (timer)
                    {
                        timer_tw.del_timer(timer);
                        users[sockfd].timer = NULL;
                    }
                }
                else
                {
                    // send( sockfd, users[sockfd].buf, BUFFER_SIZE-1, 0 );
                    if (timer)
                    {
                        refresh_timer(&users[sockfd]);
                    }
                }
            }
            else
            {
                // 其他事件（如写就绪等）在本示例里无需处理
            }
        }

        if (timeout)
        {
            // 把所有已超时的连接集中处理（tick 内部会调用回调关闭套接字）
            timer_handler();
            timeout = false;
        }
    }

    close(listenfd);  // 停止接受新连接
    close(pipefd[1]); // 关闭管道写端
    close(pipefd[0]); // 关闭管道读端
    delete[] users;   // 释放动态分配的客户端信息数组
    return 0;
}
