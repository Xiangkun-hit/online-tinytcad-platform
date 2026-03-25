#include <iostream>
#include "server.h"
#include "log.h"
#include <sys/types.h>
#include <sys/stat.h>

int main(){
    // ==========日志初始化
    // 参数说明（纯手动锁+异步模式）：
    // 1. "./tcad_webserver.log"：日志文件路径（项目根目录）
    // 2. 0：开启日志（非0则关闭）
    // 3. 8192：日志缓冲区大小
    // 4. 5000000：单日志文件最大500万行（自动分割）
    // 5. 1000：异步日志队列大小（开启异步，0=同步）
    if (!Log::get_instance()->init("./tcad_webserver.log", 0, 8192, 5000000, 1000)) {
        printf("❌ 日志模块初始化失败，服务器启动终止！\n");
        return -1;
    }
    // 打印服务器启动成功日志（INFO级别，重要事件）
    LOG_INFO("✅ TCAD WebServer 日志模块初始化成功（异步模式）");

    // 2. 初始化TCAD目录（input/output，原有逻辑）
    if (access("./tcad/input", F_OK) == -1) {
        mkdir("./tcad/input", 0755);
        LOG_INFO("TCAD输入目录不存在,已自动创建：./tcad/input");
    }
    if (access("./tcad/output", F_OK) == -1) {
        mkdir("./tcad/output", 0755);
        LOG_INFO("TCAD输出目录不存在,已自动创建：./tcad/output");
    }
    LOG_INFO("✅ TCAD目录初始化完成:./tcad/input(上传脚本）、./tcad/output(仿真结果)");

    // 创建服务器，监听 8888 端口
    Server server(8080);
    // 初始化
    if(!server.init()){
        return -1;
    }
    // 启动服务器
    server.run();

    // ==========服务器退出时优雅关闭日志 ==========
    Log::get_instance()->m_stop = true;  // 通知异步日志线程退出
    sleep(1);  // 等待异步线程刷完剩余日志
    Log::get_instance()->flush();  // 强制刷写缓冲区
    LOG_INFO("✅ TCAD WebServer 正常退出，日志模块已安全关闭");

    return 0;

}
