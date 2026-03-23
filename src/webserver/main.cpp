#include <iostream>
#include "server.h"

int main(){
    // 创建服务器，监听 8888 端口
    Server server(8080);
    // 初始化
    if(!server.init()){
        return -1;
    }
    // 启动服务器
    server.run();
    return 0;

}
