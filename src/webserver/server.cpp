#include "server.h"
#include <cstring>
#include <sys/stat.h>
#include <signal.h>
#include "http_parser.h"
#include "log.h"



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
    LOG_DEBUG("开始保存TCL文件到 ../src/tinytcad/input/simulation.tcl");
    int fd = open("../src/tinytcad/input/simulation.tcl", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0) {
        // 错误日志：文件打开失败（记录errno，便于排查）
        LOG_ERROR("TCL文件打开失败!路径:../src/tinytcad/input/simulation.tcl,errno:%d", errno);
        return false;
    }
    write(fd, content, strlen(content));
    close(fd);
    LOG_INFO("TCL文件保存成功!大小:%lu字节,路径：../src/tinytcad/input/simulation.tcl", strlen(content));
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
    // 连接建立日志（DEBUG级别：调试细节）
    LOG_DEBUG("📞 客户端连接建立,clientfd:%d", clientfd);
    signal(SIGPIPE, SIG_IGN);
    char buffer[1024] = {0};       //存放从客户端读取的数据
    int readn = recv(clientfd, buffer, sizeof(buffer)-1,0);                     //每次调用recv()的返回值
        
    if(readn <= 0){
        // 警告日志：数据读取失败（客户端断开/超时）
        LOG_WARN("❌ 客户端[%d]数据读取失败,n:%d,errno:%d,连接关闭", clientfd, readn, errno);
        close(clientfd);
        return;
    }
    LOG_DEBUG("✅ 客户端[%d]读取到数据，长度：%d字节", clientfd, readn);

    //有限状态机解析HTTP请求
    HttpRequest req;
    HTTP_CODE code = http_parse(buffer, readn, req);
    if (code == HTTP_CODE::BAD_REQUEST) {
        // 错误日志：HTTP解析失败
        LOG_ERROR("❌ 客户端[%d]HTTP请求解析失败,请求格式错误", clientfd);
        char response[1024] = {0};
        snprintf(response, sizeof(response),
            "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=utf-8\r\n\r\n<h1>请求格式错误</h1>");
        send(clientfd, response, strlen(response), 0);
        close(clientfd);
        LOG_DEBUG("客户端[%d]因解析错误，连接已关闭", clientfd);
        return;
    }
    LOG_DEBUG("✅ 客户端[%d]HTTP解析成功,方法：%s,路径：%s,请求体大小：%d字节", 
              clientfd, req.method.c_str(), req.url.c_str(), req.content_len);

    
    // char method[16] = {0};  // 请求方法 GET/POST
    // char path[256] = {0};   // 请求路径 / /index /404
    // sscanf(buffer, "%s %s", method, path);  //格式化读取请求行

    //=======动态路由
    char response[8192] = {0};
    if(code == HTTP_CODE::GET_REQUEST && req.url == "/"){
        // 调试日志：客户端请求上传首页
        LOG_DEBUG("客户端[%d]请求TCAD上传首页", clientfd);
        // TCL 文件上传表单页面
        snprintf(response, sizeof(response), 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "\r\n"
            "<h1>TCAD 仿真平台 - TCL 文件上传</h1>"
            "<form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\">"
            "<p>选择 TCL 仿真脚本：</p>"
            "<input type=\"file\" name=\"tcad_file\" accept=\".tcl\" required>"
            "<br><br>"
            "<button type=\"submit\">上传 TCAD 仿真文件</button>"
            "</form>");
    }
    else if(code == HTTP_CODE::POST_REQUEST && req.url == "/upload"){
        // 信息日志：客户端开始上传TCL文件
        LOG_INFO("📤 客户端[%d]发起TCL文件上传请求,请求体大小：%d字节", clientfd, req.content_len);
        saveTclFile(req.body.c_str());
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n\r\n"
            "<h1>✅ TCL脚本上传成功!</h1>"
            "<p>文件已保存至服务器</p>"
            "<p><a href='/'>继续上传</a></p>");    
    }
    else{
        // 警告日志：请求路径不存在（404）
        LOG_WARN("⚠️  客户端[%d]请求不存在的路径：%s,返回404", clientfd, req.url.c_str());
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
            "<h1>404 页面不存在</h1>");    
    }
    send(clientfd, response, strlen(response), 0);
    LOG_DEBUG("✅ 客户端[%d]响应发送成功，长度：%lu字节", clientfd, strlen(response));
    close(clientfd);
    LOG_DEBUG("🔌 客户端[%d]连接关闭，请求处理流程结束", clientfd);
    
    /* if(strcmp(path, "/") == 0){
        
        send(clientfd, upload_page, strlen(upload_page), 0);
        close(clientfd);
        return;
    }
    if (strcmp(path, "/upload") == 0 && strcmp(method, "POST") == 0) {
        const char* resp = 
            
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
    close(clientfd);  */ 
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