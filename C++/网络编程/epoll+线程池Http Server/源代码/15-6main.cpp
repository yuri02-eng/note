#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "14-2locker.h"     // 线程同步工具（锁、信号量、条件变量）
#include "15-3threadpool.h" // 线程池模板类
#include "15-4http_conn.h"  // HTTP连接处理类

#define MAX_FD 65536           // 最大文件描述符数（支持65536个并发连接）
#define MAX_EVENT_NUMBER 10000 // epoll最大事件数

// 外部函数声明，用于添加/移除文件描述符到epoll
extern int addfd(int epollfd, int fd, bool one_shot);
extern int removefd(int epollfd, int fd);

/*
 * 设置信号处理函数
 * sig: 信号编号
 * handler: 信号处理函数指针
 * restart: 是否重启被中断的系统调用，默认true
 */
void addsig(int sig, void(handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler; // 设置信号处理函数
    if (restart)
    {
        sa.sa_flags |= SA_RESTART; // 设置SA_RESTART标志，被信号中断的系统调用自动重启
    }
    sigfillset(&sa.sa_mask);                 // 阻塞所有其他信号
    assert(sigaction(sig, &sa, NULL) != -1); // 设置信号处理
}

/*
 * 显示错误信息并关闭连接
 * connfd: 客户端连接的文件描述符
 * info: 错误信息字符串
 */
void show_error(int connfd, const char *info)
{
    printf("%s", info);
    send(connfd, info, strlen(info), 0); // 发送错误信息给客户端
    close(connfd);                       // 关闭连接
}

/*
 * 主函数：HTTP服务器入口
 */
int main(int argc, char *argv[])
{
    // 参数检查：需要IP地址和端口号
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1]; // 服务器监听IP
    int port = atoi(argv[2]); // 服务器监听端口

    // 忽略SIGPIPE信号（防止向已关闭的socket写数据导致程序退出）
    addsig(SIGPIPE, SIG_IGN);

    // 创建线程池
    threadpool<http_conn> *pool = NULL;
    try
    {
        // 使用默认参数创建线程池（8个工作线程，最大10000个任务）
        pool = new threadpool<http_conn>;
    }
    catch (...) // 捕获所有异常
    {
        return 1; // 线程池创建失败，退出程序
    }

    // 为每个可能的文件描述符分配HTTP连接对象
    http_conn *users = new http_conn[MAX_FD];
    assert(users);
    int user_count = 0; // 当前连接数（已初始化，但代码中未使用）

    // 创建监听socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 设置SO_LINGER选项，避免TIME_WAIT状态
    struct linger tmp = {1, 0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    // 绑定地址和端口
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;              // IPv4
    inet_pton(AF_INET, ip, &address.sin_addr); // 将点分十进制IP转换为网络字节序
    address.sin_port = htons(port);            // 主机字节序转网络字节序

    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0); // 绑定失败则终止程序

    // 开始监听，最大连接队列为5
    ret = listen(listenfd, 5);
    assert(ret >= 0);

    // 创建epoll实例
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    // 将监听socket添加到epoll，不设置EPOLLONESHOT（可重复触发）
    addfd(epollfd, listenfd, false);

    // 设置HTTP连接类的静态成员（所有HTTP连接共享的epoll文件描述符）
    http_conn::m_epollfd = epollfd;

    // 主事件循环
    while (true)
    {
        // 等待事件发生，无限期阻塞
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) // 错误且不是被信号中断
        {
            printf("epoll failure\n");
            break;
        }

        // 处理所有就绪的事件
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            // 1. 新客户端连接到达
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);

                // 接受新连接
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                if (connfd < 0)
                {
                    printf("errno is: %d\n", errno);
                    continue; // 接受失败，继续处理其他事件
                }

                // 检查是否达到最大连接数
                if (http_conn::m_user_count >= MAX_FD)
                {
                    show_error(connfd, "Internal server busy");
                    continue; // 服务器繁忙，拒绝连接
                }

                // 初始化新的HTTP连接对象
                users[connfd].init(connfd, client_address);
            }
            // 2. 连接关闭或错误事件
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // 对端关闭连接、挂起或发生错误
                users[sockfd].close_conn();
            }
            // 3. 可读事件（客户端发送了数据）
            else if (events[i].events & EPOLLIN)
            {
                // 从socket读取数据
                if (users[sockfd].read())
                {
                    // 读取成功，将HTTP连接对象添加到线程池任务队列
                    pool->append(users + sockfd);
                }
                else
                {
                    // 读取失败，关闭连接
                    users[sockfd].close_conn();
                }
            }
            // 4. 可写事件（可以发送数据给客户端）
            else if (events[i].events & EPOLLOUT)
            {
                // 向socket写入数据
                if (!users[sockfd].write())
                {
                    // 写入失败，关闭连接
                    users[sockfd].close_conn();
                }
            }
            // 5. 其他事件（不处理）
            else
            {
            }
        }
    }

    // 清理资源
    close(epollfd);
    close(listenfd);
    delete[] users; // 释放HTTP连接对象数组
    delete pool;    // 释放线程池
    return 0;
}