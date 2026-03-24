#include "log.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

Log::Log(){
    m_count = 0;
    m_is_async = false;
    m_fp = nullptr;
    m_buf = nullptr;
    m_log_queue = nullptr;
    m_close_log = 0;
    m_today = 0;
}

Log::~Log(){
    m_mutex.lock();
    if(m_fp) fclose(m_fp);
    if(m_buf) delete[]m_buf;
    if(m_log_queue) delete m_log_queue;
    m_mutex.unlock();
}


//异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char* file_name, int close_log, int log_buf_size,int split_lines, int max_queue_size){
    //如果设置了max_queue_size,则设置为异步
    if(max_queue_size >= 1){
        //设置写入方式flag
        m_is_async = true;

        //创建并设置阻塞队列长度
        m_log_queue = new block_queue<std::string>(max_queue_size);
        pthread_t tid;

        //flush_log_thread为回调函数,这里表示创建线程异步写日志
        pthread_create(&tid, nullptr, flush_log_thread, nullptr);
    }

    m_close_log = close_log;
    m_log_buf_size = log_buf_size;              //输出内容的长度
    m_buf = new char[m_log_buf_size];
    memset(m_buf, 0, m_log_buf_size);

    //日志的最大行数
    m_split_lines = split_lines;

    time_t t = time(nullptr);
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    m_today = my_tm.tm_mday;

    //从后往前找到第一个/的位置
    const char* p = strrchr(file_name, '/');
    char log_full_name[256] = {0};


    //相当于自定义日志名
    //若输入的文件名没有/，则直接将时间+文件名作为日志名
    if(p == nullptr){
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", 
            my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }else{
        //将/的位置向后移动一个位置，然后复制到logname中
        //p - file_name + 1是文件所在路径文件夹的长度
        //dirname相当于./
        strcpy(log_name, p+1);
        strncpy(dir_name, file_name, p - file_name +1);

        //后面的参数跟format有关
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s",
                dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_fp = fopen(log_full_name, "a");
    if(!m_fp) return false;

    return true;
}


void Log::write_log(int level, const char* format, ...){

}

//刷新缓冲区
void Log::flush()
{

}
