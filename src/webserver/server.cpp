#include "server.h"
#include <cstring>
#include <sys/stat.h>
#include <signal.h>



bool g_server_running = true;     // 全局变量，控制服务器退出
void stopServer(int sig){
    g_server_running = false;
    std::cout << "\n服务器正在安全关闭..." << std::endl;
    // 忽略SIGPIPE信号，防止向已关闭的socket发送数据时程序崩溃
}


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
    signal(SIGINT, stopServer);      // 注册信号：Ctrl+C 关闭服务器
    signal(SIGPIPE, SIG_IGN);

    std::cout << "服务器启动成功，等待客户端连接..." << std::endl;

    while(g_server_running){       // 用全局变量控制循环，支持优雅退出
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
                // m_thread_pool->addTask(Server::handleClient, curr_fd);  //处理客户端数据
                Server::handleClient(curr_fd);
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

    // TCAD 工艺/仿真文件
    if (strstr(path, ".tcl"))   return "text/plain";    // TCAD 脚本文件
    if (strstr(path, ".dat"))   return "text/plain";    // 仿真数据文件
    if (strstr(path, ".plt"))   return "text/plain";    // 仿真绘图文件

    return "text/plain";
}

bool saveTclFile(const char* content){
    int fd = open("../src/tinytcad/input/simulation.tcl", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0) return false;

    write(fd, content, strlen(content));
    close(fd);
    return true;
}

void parseTclParams(const char* content, char* temp, char* dose, char* time){
    strcpy(temp, "unknown");
    strcpy(dose, "unknown");
    strcpy(time, "unknown");

    char t[32], d[32], ti[32];
    if (sscanf(content, "set temperature %s", t) == 1) strcpy(temp, t);
    if (sscanf(content, "set dose %s", d) == 1) strcpy(dose, d);
    if (sscanf(content, "set time %s", ti) == 1) strcpy(time, ti);
}

// 静态业务处理函数（无对象依赖，线程池安全调用）
void Server::handleClient(int clientfd){
    signal(SIGPIPE, SIG_IGN);
    char buffer[1024] = {0};       //存放从客户端读取的数据
    int readn = recv(clientfd, buffer, sizeof(buffer)-1,0);                     //每次调用recv()的返回值
        
    if(readn <= 0){
        close(clientfd);
        return;
    }

    char method[16] = {0};  // 请求方法 GET/POST
    char path[256] = {0};   // 请求路径 / /index /404
    sscanf(buffer, "%s %s", method, path);  //格式化读取请求行

    //=======动态路由
    char response[2048] = {0};
    
    if(strcmp(path, "/") == 0){
        // TCL 文件上传表单页面
        const char* upload_page = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "\r\n"
            "<h1>TCAD 仿真平台 - TCL 文件上传</h1>"
            "<form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\">"
            "<p>选择 TCL 仿真脚本：</p>"
            "<input type=\"file\" name=\"tcad_file\" accept=\".tcl\" required>"
            "<br><br>"
            "<button type=\"submit\">上传 TCAD 仿真文件</button>"
            "</form>";
        send(clientfd, upload_page, strlen(upload_page), 0);
        close(clientfd);
        return;
    }
    if (strcmp(path, "/upload") == 0 && strcmp(method, "POST") == 0) {
        const char* resp = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n\r\n"
            "<h1>✅ TCL脚本上传成功！</h1>"
            "<p>文件已保存至服务器</p>"
            "<p><a href='/'>继续上传</a></p>";
        send(clientfd, resp, strlen(resp), 0);
        close(clientfd);
        return;
    }
    if(strstr(path, "/api")){
        //解析GET参数
        char name[32] = "unknown";
        sscanf(path, "/api?name=%s", name);
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            "<p>dynamic: hello! %s</p>", name);
        send(clientfd, response, strlen(response), 0);
        close(clientfd);
        return;    
    }

    //=====静态文件
    char filePath[512] = "../src/tinytcad/input";   //输入文件目录
    strcat(filePath, path);
    
    struct stat fileStat;
    if(stat(filePath, &fileStat) < 0){
        // 文件不存在：返回404
        char notFound[] = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: text/html\r\n"
                          "\r\n"
                          "<h1>noFile404 Not Found</h1>";
        send(clientfd, notFound, sizeof(notFound), 0);
        close(clientfd);
        return;
    }
    if(stat(filePath, &fileStat) == 0){
        int fd = open(filePath, O_RDONLY);

        char header[1024] = {0};
        // snprintf：安全函数，指定缓冲区大小，永不溢出
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "\r\n",
            getContentType(filePath),
            fileStat.st_size    // 文件真实大小
        );
        send(clientfd, header, strlen(header), 0);    //  先发送响应头

        char fileBuffer[4096] = {0};
        ssize_t len;
        while((len = read(fd, fileBuffer, sizeof(fileBuffer))) > 0){
            send(clientfd, fileBuffer, len, 0);
        }                     // 循环读取文件 → 循环发送，支持任意大小文件
        close(fd);
        close(clientfd);      // 短连接：发送完关闭（HTTP/1.1默认短连接）
        return;
    }
    snprintf(response, sizeof(response),
        "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
        "<h1>404 Not Found</h1>");
    send(clientfd, response, strlen(response), 0);
    close(clientfd);   
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