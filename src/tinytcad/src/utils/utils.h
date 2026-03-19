// src/utils/utils.h
// 工具模块：日志打印、程序初始化
// TCAD微型仿真框架
#ifndef TINYTCAD_UTILS_H
#define TINYTCAD_UTILS_H

#include <string>

class Utils {
public:
  Utils() = default;
  ~Utils() = default;

  //初始化函数
  void init();

  //打印普通日志信息
  void log_info(const char* msg) const;
  //成功信息
  void log_success(const char* msg) const;

  // 打印错误信息（预留）
  void log_error(const char* msg) const;

private:
  // 获取系统时间字符串（给日志加时间戳）
  std::string get_time_str() const;
};

#endif