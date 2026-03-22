#include "server.h"

// 构造函数：指定端口
Server::Server(int port) : m_port(port), m_listenfd(-1), m_epollfd(-1){

}

// 服务器初始化：socket -> bind -> listen
bool Server::init(){
    // 1. 创建套接字（TCP协议）
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_listenfd == -1){
        std::cerr << "socket 创建失败！" << std::endl;
        return false;
    }

    // 2. 绑定端口和IP
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);

    if(bind(m_listenfd,(struct sockaddr*)&address,sizeof(address)) == -1){
        std::cerr << "bind failed" << std::endl;
        close(m_listenfd);
        return false;       
    }

    // 3. 监听（最大等待连接数5）
    if(listen(m_listenfd, 5) == -1){
        std::cerr << "listen 监听失败！" << std::endl;
        close(m_listenfd);
        return false;
    }

    //初始化 Epoll
    if(!initEpoll()){
        close(m_listenfd);
        return false;
    }

    //将监听fd添加到epoll
    addFdToEpoll(m_listenfd);

    std::cout << "服务器初始化成功！监听端口：" << m_port << std::endl;
    return true;
}

// 启动服务器，接收连接
void Server::run(){
    std::cout << "服务器启动成功，等待客户端连接..." << std::endl;

    while(true){
        // 阻塞等待事件发生（超时-1表示永久阻塞）
        int event_count = epoll_wait(m_epollfd, m_events, MAX_EVENTS, -1);
        if(event_count == -1){
            std::cerr << "epoll_wait 失败！" << std::endl;
            continue;
        }

        // 遍历所有触发的事件
        for(int i = 0; i< event_count; ++i){
            int curr_fd = m_events[i].data.fd;

            //情况1：监听fd触发 → 有新客户端连接
            if(curr_fd == m_listenfd){
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientfd = accept(m_listenfd, (sockaddr*)&client, &len);
                if(clientfd == -1) continue;

                std::cout << "新客户端连接!fd:" << clientfd << std::endl;
                addFdToEpoll(clientfd);   //将新连接的客户端也加入epoll监听
            }
             // 情况2：客户端fd触发 → 有数据可读
            else{
                std::cout << "客户端fd " << curr_fd << " 发送数据" << std::endl;
                // 暂时关闭连接（下一步实现数据读取）
                close(curr_fd);
            }

        }

    }

}

//epoll init
bool Server::initEpoll(){
    //创建 epoll 句柄，参数>0即可（兼容旧内核）
    m_epollfd = epoll_create1(0);
    if(m_epollfd == -1){
        std::cerr << "epoll_create 创建失败！" << std::endl;
        return false;
    }
    std::cout << "epoll 初始化成功!epollfd: " << m_epollfd << std::endl;
    return true;
}

//封装「将fd添加到epoll」的工具函数
void Server::addFdToEpoll(int fd){
    struct epoll_event ev;
    ev.events = EPOLLIN;    // 监听 读事件
    ev.data.fd = fd;        // 绑定文件描述符

    // EPOLL_CTL_ADD：添加fd和socket到epoll句柄
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev);
}

// 析构函数：关闭套接字
Server::~Server(){
    if(m_listenfd != -1){
        close(m_listenfd);
    }
    if(m_epollfd != -1){
        close(m_epollfd);
    }

}