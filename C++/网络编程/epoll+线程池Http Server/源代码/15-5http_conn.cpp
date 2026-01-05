#include "15-4http_conn.h"

/**
 * HTTP响应状态码和对应的描述信息
 */
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";
const char *doc_root = "/var/www/html"; // 网站根目录

/**
 * 设置文件描述符为非阻塞模式
 * @param fd 要设置的文件描述符
 * @return 旧的文件描述符状态
 */
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);      // 获取当前文件状态标志
    int new_option = old_option | O_NONBLOCK; // 添加非阻塞标志
    fcntl(fd, F_SETFL, new_option);           // 设置新的文件状态标志
    return old_option;                        // 返回旧的状态，便于后续恢复
}

/**
 * 向epoll实例注册文件描述符
 * @param epollfd epoll文件描述符
 * @param fd 要注册的文件描述符
 * @param one_shot 是否启用EPOLLONESHOT事件
 */
void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    // 设置事件类型：可读、边缘触发、对端关闭连接
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (one_shot)
    {
        // 添加EPOLLONESHOT，确保同一时间只有一个线程处理该socket
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event); // 注册事件
    setnonblocking(fd);                            // 设置为非阻塞模式（ET模式必须）
}

/**
 * 从epoll中移除文件描述符并关闭连接
 * @param epollfd epoll文件描述符
 * @param fd 要移除的文件描述符
 */
void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0); // 从epoll中删除
    close(fd);                                // 关闭socket连接
}

/**
 * 修改epoll中文件描述符的事件类型
 * @param epollfd epoll文件描述符
 * @param fd 要修改的文件描述符
 * @param ev 新的事件类型
 */
void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    // 设置新的事件类型，保持边缘触发和ONESHOOT特性
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event); // 修改事件
}

// 静态成员变量初始化
int http_conn::m_user_count = 0; // 初始连接数为0
int http_conn::m_epollfd = -1;   // 初始epollfd为无效值

/**
 * 关闭HTTP连接
 * @param real_close 是否真正关闭连接（false时只重置状态）
 */
void http_conn::close_conn(bool real_close)
{
    if (real_close && (m_sockfd != -1))
    {
        // 从epoll中移除并关闭socket
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;  // 标记socket为已关闭
        m_user_count--; // 减少用户连接计数
    }
}

/**
 * 初始化新的HTTP连接
 * @param sockfd 客户端socket文件描述符
 * @param addr 客户端地址信息
 */
void http_conn::init(int sockfd, const sockaddr_in &addr)
{
    m_sockfd = sockfd; // 保存socket描述符
    m_address = addr;  // 保存客户端地址

    // 获取socket错误状态并清空错误队列
    int error = 0;
    socklen_t len = sizeof(error);
    getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len);

    // 设置地址重用选项，避免TIME_WAIT状态
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 将socket添加到epoll监控
    addfd(m_epollfd, sockfd, true);
    m_user_count++; // 增加用户连接计数

    init(); // 调用私有init方法初始化其他状态
}

/**
 * 初始化HTTP连接状态（重置所有状态变量）
 */
void http_conn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE; // 初始状态：解析请求行
    m_linger = false;                        // 默认不保持连接

    // 重置HTTP请求解析相关变量
    m_method = GET;       // 默认GET方法
    m_url = 0;            // 请求URL指针
    m_version = 0;        // HTTP版本指针
    m_content_length = 0; // 内容长度
    m_host = 0;           // 主机名指针
    m_start_line = 0;     // 行起始位置
    m_checked_idx = 0;    // 当前解析位置
    m_read_idx = 0;       // 已读数据长度
    m_write_idx = 0;      // 待发送数据长度

    // 清空缓冲区
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

/**
 * 解析一行数据，查找\r\n行结束符
 * @return 行解析状态（LINE_OK, LINE_BAD, LINE_OPEN）
 */
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    // 遍历已读入的数据，查找行结束符
    for (; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        temp = m_read_buf[m_checked_idx]; // 获取当前字符

        if (temp == '\r') // 遇到回车符
        {
            if ((m_checked_idx + 1) == m_read_idx)
            {
                return LINE_OPEN; // 数据不完整，需要继续读取
            }
            else if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                // 找到完整的\r\n，替换为字符串结束符
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK; // 成功解析一行
            }
            return LINE_BAD; // 格式错误
        }
        else if (temp == '\n') // 遇到换行符（处理可能的非标准格式）
        {
            if ((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r'))
            {
                // 找到\n，且前一个字符是\r
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD; // 格式错误
        }
    }

    return LINE_OPEN; // 需要继续读取数据
}

/**
 * 从socket读取数据到读缓冲区（非阻塞读取）
 * @return 读取是否成功
 */
bool http_conn::read()
{
    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        return false; // 读缓冲区已满
    }

    int bytes_read = 0;
    // 循环读取，直到没有数据可读或缓冲区满
    while (true)
    {
        // 从socket读取数据到读缓冲区空闲部分
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx,
                          READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // 没有数据可读了（非阻塞IO的正常情况）
            }
            return false; // 读取错误
        }
        else if (bytes_read == 0)
        {
            return false; // 对端关闭连接
        }

        m_read_idx += bytes_read; // 更新已读数据长度
    }
    return true;
}

/**
 * 解析HTTP请求行（如：GET /index.html HTTP/1.1）
 * @param text 要解析的请求行字符串
 * @return HTTP处理结果代码
 */
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    // 查找第一个空格或制表符，分隔方法和URL
    m_url = strpbrk(text, " \t");
    if (!m_url)
    {
        return BAD_REQUEST; // 格式错误：没有空格分隔符
    }
    *m_url++ = '\0'; // 截断方法部分，m_url指向URL开始

    // 解析HTTP方法
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
    {
        m_method = GET; // 目前只支持GET方法
    }
    else
    {
        return BAD_REQUEST; // 不支持的方法
    }

    // 跳过URL前的空白字符
    m_url += strspn(m_url, " \t");
    // 查找URL和版本号之间的分隔符
    m_version = strpbrk(m_url, " \t");
    if (!m_version)
    {
        return BAD_REQUEST; // 格式错误
    }
    *m_version++ = '\0';                   // 截断URL，m_version指向版本号开始
    m_version += strspn(m_version, " \t"); // 跳过空白字符

    // 检查HTTP版本（只支持HTTP/1.1）
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }

    // 处理URL（支持http://前缀）
    if (strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;                 // 跳过"http://"
        m_url = strchr(m_url, '/'); // 查找路径开始位置
    }

    // URL验证：必须以/开头
    if (!m_url || m_url[0] != '/')
    {
        return BAD_REQUEST;
    }

    // 状态转移：下一步解析头部
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST; // 请求解析还未完成
}

/**
 * 解析HTTP请求头部
 * @param text 要解析的头部行
 * @return HTTP处理结果代码
 */
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    if (text[0] == '\0') // 空行，头部结束
    {
        if (m_method == HEAD) // HEAD方法不需要内容体
        {
            return GET_REQUEST;
        }

        // 如果有消息体，需要继续读取内容
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT; // 状态转移
            return NO_REQUEST;
        }

        return GET_REQUEST; // 获得完整请求
    }
    else if (strncasecmp(text, "Connection:", 11) == 0) // Connection头部
    {
        text += 11;
        text += strspn(text, " \t"); // 跳过空白
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true; // 设置保持连接标志
        }
    }
    else if (strncasecmp(text, "Content-Length:", 15) == 0) // 内容长度
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text); // 转换字符串为整数
    }
    else if (strncasecmp(text, "Host:", 5) == 0) // 主机名
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text; // 保存主机名
    }
    else
    {
        printf("oop! unknow header %s\n", text); // 忽略未知头部
    }

    return NO_REQUEST; // 继续解析头部
}

/**
 * 解析HTTP请求内容体（用于POST等有消息体的请求）
 * @param text 内容体数据
 * @return HTTP处理结果代码
 */
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    // 检查是否已经读取了完整的内容体
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0'; // 添加字符串结束符
        return GET_REQUEST;            // 获得完整请求
    }

    return NO_REQUEST; // 内容体不完整，需要继续读取
}

/**
 * 处理HTTP请求的入口函数
 * @return HTTP处理结果代码
 */
http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK; // 行解析状态
    HTTP_CODE ret = NO_REQUEST;        // HTTP处理结果
    char *text = 0;                    // 当前解析的行

    // 主解析循环：有限状态机实现
    while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) ||
           ((line_status = parse_line()) == LINE_OK))
    {
        text = get_line();                     // 获取当前要解析的行
        m_start_line = m_checked_idx;          // 更新下一行的起始位置
        printf("got 1 http line: %s\n", text); // 调试输出

        // 根据当前状态机状态进行相应处理
        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE: // 解析请求行状态
        {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST; // 请求格式错误
            }
            break;
        }
        case CHECK_STATE_HEADER: // 解析头部状态
        {
            ret = parse_headers(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (ret == GET_REQUEST) // 获得完整请求
            {
                return do_request(); // 处理具体请求
            }
            break;
        }
        case CHECK_STATE_CONTENT: // 解析内容体状态
        {
            ret = parse_content(text);
            if (ret == GET_REQUEST)
            {
                return do_request();
            }
            line_status = LINE_OPEN; // 内容体需要特殊处理
            break;
        }
        default:
        {
            return INTERNAL_ERROR; // 内部状态错误
        }
        }
    }

    return NO_REQUEST; // 请求不完整，需要继续读取
}

/**
 * 处理具体的文件请求
 * @return HTTP处理结果代码
 */
http_conn::HTTP_CODE http_conn::do_request()
{
    // 构建完整的文件路径：网站根目录 + 请求URL
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);

    // 获取文件状态信息
    if (stat(m_real_file, &m_file_stat) < 0)
    {
        return NO_RESOURCE; // 文件不存在
    }

    // 检查文件读取权限
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        return FORBIDDEN_REQUEST; // 没有读取权限
    }

    // 检查是否是目录（不支持目录浏览）
    if (S_ISDIR(m_file_stat.st_mode))
    {
        return BAD_REQUEST; // 不能请求目录
    }

    // 使用内存映射提高文件读取性能
    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd); // 映射完成后可以关闭文件描述符

    return FILE_REQUEST; // 文件请求处理成功
}

/**
 * 释放内存映射的文件
 */
void http_conn::unmap()
{
    if (m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size); // 解除内存映射
        m_file_address = 0;                          // 重置指针
    }
}

/**
 * 向客户端发送数据
 * @return 发送是否成功
 */
bool http_conn::write()
{
    int temp = 0;
    int bytes_have_send = 0;         // 已发送字节数
    int bytes_to_send = m_write_idx; // 待发送字节数

    // 如果没有数据要发送，重新注册读事件
    if (bytes_to_send == 0)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN); // 监听读事件
        init();                              // 重置连接状态
        return true;
    }

    // 循环发送数据，直到全部发送完成或发生错误
    while (1)
    {
        // 使用writev进行分散写（聚合写）
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if (temp <= -1)
        {
            if (errno == EAGAIN) // 写缓冲区满，稍后重试
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT); // 重新注册写事件
                return true;
            }
            unmap(); // 发送失败，释放资源
            return false;
        }

        // 更新发送统计
        bytes_to_send -= temp;
        bytes_have_send += temp;

        // 检查是否全部发送完成
        if (bytes_to_send <= bytes_have_send)
        {
            unmap();      // 发送完成，释放内存映射
            if (m_linger) // 如果是持久连接
            {
                init();                              // 重置连接状态，准备处理下一个请求
                modfd(m_epollfd, m_sockfd, EPOLLIN); // 监听读事件
                return true;
            }
            else // 非持久连接
            {
                modfd(m_epollfd, m_sockfd, EPOLLIN); // 监听读事件（可能关闭）
                return false;                        // 通知上层关闭连接
            }
        }
    }
}

/**
 * 向写缓冲区添加格式化响应内容（支持可变参数）
 * @param format 格式化字符串
 * @param ... 可变参数
 * @return 添加是否成功
 */
bool http_conn::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
    {
        return false; // 写缓冲区已满
    }

    // 使用可变参数处理格式化输出
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx,
                        WRITE_BUFFER_SIZE - 1 - m_write_idx,
                        format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        return false; // 输出被截断
    }
    m_write_idx += len; // 更新写索引
    va_end(arg_list);
    return true;
}

/**
 * 添加HTTP状态行（如：HTTP/1.1 200 OK）
 * @param status 状态码
 * @param title 状态描述
 * @return 添加是否成功
 */
bool http_conn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
bool http_conn::add_headers(int content_len)
{
    // 添加完整的HTTP响应头，包括Content-Length、Connection和空行
    add_content_length(content_len); // 添加内容长度头
    add_linger();                    // 添加连接状态头
    add_blank_line();                // 添加头部结束空行
    return true;
}

/**
 * 添加Content-Length头部字段
 * @param content_len 内容长度
 * @return 添加是否成功
 */
bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length: %d\r\n", content_len);
}

/**
 * 添加Connection头部字段（保持连接或关闭连接）
 * @return 添加是否成功
 */
bool http_conn::add_linger()
{
    return add_response("Connection: %s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

/**
 * 添加HTTP头部结束的空行（分隔头部和实体主体）
 * @return 添加是否成功
 */
bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

/**
 * 添加HTTP响应实体内容
 * @param content 要添加的内容字符串
 * @return 添加是否成功
 */
bool http_conn::add_content(const char *content)
{
    return add_response("%s", content);
}

/**
 * 根据HTTP处理结果生成相应的HTTP响应
 * @param ret HTTP处理结果代码
 * @return 响应生成是否成功
 */
bool http_conn::process_write(HTTP_CODE ret)
{
    // 根据不同的处理结果生成相应的HTTP响应
    switch (ret)
    {
    case INTERNAL_ERROR: // 服务器内部错误（500）
    {
        add_status_line(500, error_500_title); // 状态行：HTTP/1.1 500 Internal Error
        add_headers(strlen(error_500_form));   // 添加响应头
        if (!add_content(error_500_form))      // 添加错误页面内容
        {
            return false;
        }
        break;
    }
    case BAD_REQUEST: // 客户端请求错误（400）
    {
        add_status_line(400, error_400_title); // 状态行：HTTP/1.1 400 Bad Request
        add_headers(strlen(error_400_form));   // 添加响应头
        if (!add_content(error_400_form))      // 添加错误页面内容
        {
            return false;
        }
        break;
    }
    case NO_RESOURCE: // 资源不存在（404）
    {
        add_status_line(404, error_404_title); // 状态行：HTTP/1.1 404 Not Found
        add_headers(strlen(error_404_form));   // 添加响应头
        if (!add_content(error_404_form))      // 添加错误页面内容
        {
            return false;
        }
        break;
    }
    case FORBIDDEN_REQUEST: // 禁止访问（403）
    {
        add_status_line(403, error_403_title); // 状态行：HTTP/1.1 403 Forbidden
        add_headers(strlen(error_403_form));   // 添加响应头
        if (!add_content(error_403_form))      // 添加错误页面内容
        {
            return false;
        }
        break;
    }
    case FILE_REQUEST: // 文件请求成功（200）
    {
        add_status_line(200, ok_200_title); // 状态行：HTTP/1.1 200 OK
        if (m_file_stat.st_size != 0)       // 文件不为空
        {
            add_headers(m_file_stat.st_size); // 添加响应头（文件大小作为内容长度）

            // 设置分散写（writev）参数
            m_iv[0].iov_base = m_write_buf;        // 第一个内存块：HTTP响应头
            m_iv[0].iov_len = m_write_idx;         // 响应头长度
            m_iv[1].iov_base = m_file_address;     // 第二个内存块：文件内容（内存映射）
            m_iv[1].iov_len = m_file_stat.st_size; // 文件大小
            m_iv_count = 2;                        // 两个内存块
            return true;
        }
        else // 文件为空
        {
            // 发送空文件的默认响应
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string)); // 添加响应头
            if (!add_content(ok_string))    // 添加默认内容
            {
                return false;
            }
        }
    }
    default:
    {
        return false; // 未知的处理结果
    }
    }

    // 对于错误响应，只需要发送响应头（没有文件内容）
    m_iv[0].iov_base = m_write_buf; // 响应头缓冲区
    m_iv[0].iov_len = m_write_idx;  // 响应头长度
    m_iv_count = 1;                 // 只有一个内存块（只有响应头）
    return true;
}

/**
 * HTTP连接处理的主函数（线程池工作线程调用的入口）
 * 处理流程：读取请求 → 解析请求 → 生成响应 → 发送响应
 */
void http_conn::process()
{
    // 步骤1：读取并解析HTTP请求
    HTTP_CODE read_ret = process_read();

    // 如果请求不完整，需要继续读取数据
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN); // 重新注册读事件，等待更多数据
        return;
    }

    // 步骤2：根据解析结果生成HTTP响应
    bool write_ret = process_write(read_ret);

    // 如果响应生成失败，关闭连接
    if (!write_ret)
    {
        close_conn(); // 关闭HTTP连接
    }

    // 步骤3：注册写事件，准备发送HTTP响应
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}