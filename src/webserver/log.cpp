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
    if(m_close_log) return;

    struct timeval now{};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    char level_str[16] = {0};

    switch(level){
        case 0:
            strcpy(level_str, "[DEBUG]");
            break;
        case 1:
            strcpy(level_str, "[INFO]");
            break;
        case 2:
            strcpy(level_str, "[WARN]");
            break;
        case 3:
            strcpy(level_str, "[ERROR]");
        default:
            strcpy(level_str, "[INFO]");
            break;
    }

    m_mutex.lock();

    //更新现有行数
    m_count++;

    //日志不是今天或写入的日志行数是最大行的倍数
    //m_split_lines为最大行数
    // 日志切分判断：满足任意一个条件就切分文件
    // 条件1：日期变了（跨天）  条件2：当前行数是最大行数的整数倍（文件写满了）
    if(m_today != my_tm.tm_mday || m_count % m_split_lines == 0)
    {
        char new_log[256] = {0};
        fflush(m_fp);         // 刷新缓冲区：把内存里的日志强制写入旧文件
        fclose(m_fp);         // 关闭旧的日志文件
        char tail[16] = {0};

        //格式化日志名中的时间部分
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        // 分支1：日期变了 → 新建【当天】的日志文件
        if(m_today != my_tm.tm_mday){
            // 拼接新日志文件名：目录+时间+日志名
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        // 分支2：文件写满了 → 新建【分卷】日志文件
        else{
            // 文件名后缀加数字：2026_03_24_server.log.1 / .2 ...
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();

    va_list valst;
    //将传入的format参数赋值给valst，便于格式化输出  初始化可变参数
    va_start(valst, format);

    std::string log_str;

    m_mutex.lock();

    //写入内容格式：时间 + 内容
    //时间格式化，snprintf成功返回写字符的总数，其中不包括结尾的null字符
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                      my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                      my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, level_str);
    //内容格式化，用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数(不包含终止符)
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    
    log_str = m_buf;
    m_mutex.unlock();
    va_end(valst);    //释放可变参数

    //若m_is_async为true表示异步，默认为同步
    //若异步,则将日志信息加入阻塞队列,同步则加锁向文件中写
    if(m_is_async && !m_log_queue->full()){
        m_log_queue->push(log_str);
    }else{
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }
}

//刷新缓冲区
void Log::flush()
{
    if(m_close_log) return;
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}
