#include "log.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

Log::Log(){

}

Log::~Log(){

}

// bool Log::init(const char* file_name, int close_log = 0, int log_buf_size = 8192,
//               int split_lines = 5000000, int max_queue_size = 0){

//               }


void Log::write_log(int level, const char* format, ...){

}

//刷新缓冲区
void Log::flush()
{

}
