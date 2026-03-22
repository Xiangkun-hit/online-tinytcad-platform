#ifndef SERVER_H
#define SERVER_H

#include <iostream>
//needed header
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
//epoll
#include <sys/epoll.h>
#define MAX_EVENTS 1024

// WebServer 基础类
class Server{
private:
    int m_listenfd;  //监听socket
    int m_port;      //端口号

    int m_epollfd;   //epoll 文件描述符
    struct epoll_event m_events[MAX_EVENTS];  //存储epoll返回的事件数组

public:
    // 构造函数：指定端口
    Server(int port);
    // 初始化服务器
    bool init();
    // 启动服务器，接收连接
    void run();

    //epoll init
    bool initEpoll();
    //封装「将fd添加到epoll」的工具函数
    void addFdToEpoll(int fd);

    // 析构函数：关闭套接字
    ~Server();
};


#endif