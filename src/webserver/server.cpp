#include "server.h"
#include <cstring>
#include <sys/stat.h>


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

    // 端口复用
    int reuse = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

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

    m_thread_pool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());   // 初始化线程池（自动获取CPU核心数）

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
                while(true){
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int clientfd = accept(m_listenfd, (sockaddr*)&client, &len);
                    if(clientfd < 0){
                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                            break;
                        }
                        std::cerr << "accept failed\n";
                        break;
                    } 

                    std::cout << "新客户端连接!fd:" << clientfd << std::endl;
                    addFdToEpoll(clientfd);   //将新连接的客户端也加入epoll监听
                }
                
            }
             // 情况2：客户端fd触发 → 有数据可读
            else{
                std::cout << "客户端fd " << curr_fd << " 发送数据" << std::endl;
                m_thread_pool->addTask(Server::handleClient, curr_fd);  //处理客户端数据
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


static int setNonBlock(int fd){
    int old_flag = fcntl(fd, F_GETFL, 0);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return old_flag;
}


//封装「将fd添加到epoll」的工具函数
bool Server::addFdToEpoll(int fd){
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;    // 监听 读事件 ET模式
    ev.data.fd = fd;        // 绑定文件描述符

    // if(oneshot){
    //     ev.events |= EPOLLONESHOT;  //事件只触发一次
    // }

    // EPOLL_CTL_ADD：添加fd和event到epoll句柄
    if(epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev) == -1){
        std::cerr << "epoll_ctl add failed\n";
        return false;
    }

    setNonBlock(fd);
    return true;
}

const char* getContentType(const char* path){
    if(strstr(path, ".html")) return "text/html";
    if (strstr(path, ".jpg"))  return "image/jpeg";
    if (strstr(path, ".png"))  return "image/png";
    if (strstr(path, ".css"))  return "text/css";
    return "text/plain";
}

// 静态业务处理函数（无对象依赖，线程池安全调用）
void Server::handleClient(int clientfd){
    char buffer[1024] = {0};       //存放从客户端读取的数据
    int readn = recv(clientfd, buffer, sizeof(buffer)-1,0);                     //每次调用recv()的返回值
        
    if(readn <= 0){
        close(clientfd);
        return;
    }

    char method[16] = {0};  // 请求方法 GET/POST
    char path[256] = {0};   // 请求路径 / /index /404
    sscanf(buffer, "%s %s", method, path);  //格式化读取请求行

    char filePath[512] = "../www";
    if(strcmp(path, "/") == 0 ){
        strcat(filePath, "/index.html");
    }else{
        strcat(filePath, path);
    }

    struct stat fileStat;
    if(stat(filePath, &fileStat) < 0){
        // 文件不存在：返回404
        char notFound[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>noFile404 Not Found</h1>";
        send(clientfd, notFound, sizeof(notFound), 0);
        close(clientfd);
        return;
    }

    int fd = open(filePath, O_RDONLY);
    char fileBuffer[4096] = {0};
    read(fd, fileBuffer, sizeof(fileBuffer)-1);
    close(fd);

    char response[8192] = {0};
    
    // 首页：返回HTML页面
    sprintf(response, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "\r\n"
        "%s",
        getContentType(filePath),
        fileBuffer
    );
    
    
    send(clientfd, response, strlen(response), 0);    // 发送HTTP响应给浏览器
    close(clientfd);      // 短连接：发送完关闭（HTTP/1.1默认短连接）
    
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