#ifndef SERVER_H
#define SERVER_H

#include <iostream>
//needed header
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
//epoll
#include <sys/epoll.h>
#include <fcntl.h>  //非阻塞socket
#include <memory>   //智能指针
//threadpool
#include "threadpool.h"

#define MAX_EVENTS 1024
#define LISTEN_BACKLOG 512  //监听队列

// WebServer 基础类
class Server{
private:
    int m_listenfd;  //监听socket
    int m_port;      //端口号

    int m_epollfd;   //epoll 文件描述符
    struct epoll_event m_events[MAX_EVENTS];  //存储epoll返回的事件数组

    std::unique_ptr<ThreadPool> m_thread_pool;  //线程池指针

public:
    
    explicit Server(int port);   // 构造函数：指定端口
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;  // 禁用拷贝（服务器独占资源）
    ~Server();   // 析构函数：关闭套接字


    bool init();  // 初始化服务器
    void run();   // 启动服务器，接收连接
    
private: //工具函数 
    bool initEpoll();   //epoll init
    bool addFdToEpoll(int fd);   //封装「将fd添加到epoll」的工具函数
    static void handleClient(int clientfd);  //处理客户端数据的函数
       
};


#endif