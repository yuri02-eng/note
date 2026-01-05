#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "14-2locker.h"

/**
 * HTTP连接处理类
 * 负责处理单个HTTP连接的完整生命周期：接收请求、解析协议、生成响应
 * 采用状态机模式解析HTTP协议，支持keep-alive持久连接
 */
class http_conn
{
public:
    // 静态常量定义
    static const int FILENAME_LEN = 200;        // 文件路径最大长度
    static const int READ_BUFFER_SIZE = 2048;  // 读缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024; // 写缓冲区大小
    
    // HTTP方法枚举（目前主要支持GET）
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    
    // 解析状态：有限状态机三种状态
    enum CHECK_STATE { 
        CHECK_STATE_REQUESTLINE = 0,  // 正在解析请求行
        CHECK_STATE_HEADER,           // 正在解析头部
        CHECK_STATE_CONTENT           // 正在解析内容体
    };
    
    // 服务器处理结果枚举
    enum HTTP_CODE { 
        NO_REQUEST,         // 请求不完整，需要继续读取
        GET_REQUEST,        // 获得完整请求
        BAD_REQUEST,        // 客户端请求语法错误
        NO_RESOURCE,        // 资源不存在
        FORBIDDEN_REQUEST,  // 客户端对资源没有足够权限
        FILE_REQUEST,       // 文件请求，需要发送文件
        INTERNAL_ERROR,     // 服务器内部错误
        CLOSED_CONNECTION   // 客户端已关闭连接
    };
    
    // 行解析状态
    enum LINE_STATUS { 
        LINE_OK = 0,    // 读取到完整行
        LINE_BAD,       // 行格式错误
        LINE_OPEN       // 行数据不完整
    };

public:
    http_conn(){}   // 构造函数
    ~http_conn(){}  // 析构函数

public:
    // 公共接口方法
    void init(int sockfd, const sockaddr_in& addr);  // 初始化连接
    void close_conn(bool real_close = true);        // 关闭连接
    void process();          // 处理HTTP请求（主处理函数）
    bool read();            // 读取客户端数据
    bool write();           // 向客户端发送数据

private:
    // 内部私有方法
    void init();                            // 初始化连接状态
    HTTP_CODE process_read();               // 解析HTTP请求
    bool process_write(HTTP_CODE ret);      // 生成HTTP响应
    
    // HTTP协议解析相关方法
    HTTP_CODE parse_request_line(char* text);   // 解析请求行
    HTTP_CODE parse_headers(char* text);        // 解析头部字段
    HTTP_CODE parse_content(char* text);        // 解析内容体
    HTTP_CODE do_request();                     // 处理具体请求
    char* get_line() { return m_read_buf + m_start_line; }  // 获取当前行
    LINE_STATUS parse_line();                   // 解析一行数据
    
    // 内存管理和响应生成
    void unmap();                              // 释放内存映射
    bool add_response(const char* format, ...); // 添加响应内容（可变参数）
    bool add_content(const char* content);     // 添加响应体
    bool add_status_line(int status, const char* title); // 添加状态行
    bool add_headers(int content_length);     // 添加响应头
    bool add_content_length(int content_length); // 添加Content-Length头
    bool add_linger();                        // 添加Connection头
    bool add_blank_line();                    // 添加空行

public:
    // 静态成员变量（所有HTTP连接共享）
    static int m_epollfd;     // epoll文件描述符
    static int m_user_count;  // 当前用户连接数

private:
    // 连接相关成员变量
    int m_sockfd;              // 该HTTP连接的socket
    sockaddr_in m_address;     // 客户端socket地址
    
    // 读缓冲区及相关状态
    char m_read_buf[READ_BUFFER_SIZE];  // 读缓冲区
    int m_read_idx;             // 标识读缓冲区中已经读入的客户端数据的最后一个字节的下一个位置
    int m_checked_idx;          // 当前正在分析的字符在读缓冲区中的位置
    int m_start_line;           // 当前正在解析的行的起始位置
    
    // 写缓冲区及相关状态
    char m_write_buf[WRITE_BUFFER_SIZE]; // 写缓冲区
    int m_write_idx;            // 写缓冲区中待发送的字节数
    
    // HTTP协议解析状态
    CHECK_STATE m_check_state;  // 当前主状态机状态
    METHOD m_method;            // 请求方法
    
    // 解析出的HTTP请求信息
    char m_real_file[FILENAME_LEN]; // 客户端请求的完整路径
    char* m_url;                // 请求的URL
    char* m_version;            // HTTP版本
    char* m_host;               // 主机名
    int m_content_length;       // 内容长度
    bool m_linger;              // 是否保持连接
    
    // 文件传输相关
    char* m_file_address;       // 内存映射的文件地址
    struct stat m_file_stat;    // 文件状态信息
    struct iovec m_iv[2];      // 分散写内存结构（响应头和文件内容）
    int m_iv_count;            // 分散写内存块数量
};

#endif