#include "utils.h"
#include <iostream>
#include <ctime>
#include <iomanip>

// 颜色定义（ANSI 标准，Linux/Mac 通用）
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define WHITE   "\033[37m"
#define RED     "\033[31m"

std::string Utils::get_time_str() const{
  std::time_t now = std::time(nullptr);
  std::tm local_tm = *std::localtime(&now);
  char buf[64];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_tm);
  return std::string(buf);
}

void Utils::init() {
  // 调用自身的日志打印
  log_info("初始化模块成功");
}

//打印普通日志信息(WHITE)
void Utils::log_info(const char* msg) const{
  // 专业格式输出 [模块名] 信息
  std::cout << WHITE << "[" << get_time_str() << "] [Utils INFO] " << msg << std::endl;
}

void Utils::log_success(const char* msg) const{
  std::cout << GREEN << "[" << get_time_str() << "] [Utils SUCCESS] " << msg << std::endl;
}

// 打印错误信息（预留）
void Utils::log_error(const char* msg) const{
  std::cout << RED << "[" << get_time_str() << "] [Utils ERROR]" << msg << std::endl;
}