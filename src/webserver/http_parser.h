#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>

enum class CHECK_STATE{              // 主状态机：解析HTTP的三个阶段
    CHECK_STATE_REQUEST_LINE = 0,    // 解析请求行
    CHECK_STATE_HEADER,              // 解析请求头
    CHECK_STATE_CONTENT              // 解析请求体
};

//从状态机 行解析状态
enum class LINE_STATUS{
    LINE_OK = 0,    // 完整行 \r\n
    LINE_BAD,       // 行错误
    LINE_OPEN       // 行未读完
};

// 解析结果
enum class HTTP_CODE{
    NO_REQUEST,     // 请求不完整
    GET_REQUEST,    // 完成GET解析
    POST_REQUEST,   // 完成POST解析
    BAD_REQUEST     // 请求错误
};

// 解析后的HTTP请求结构体
struct HttpRequest{
    std::string method;     // GET/POST    
    std::string url;        // 请求路径    
    std::string body;       // POST数据（TCL文件内容）
    int content_len = 0;    // 请求体长度
};

// ======================== 对外接口函数 ========================
// 主解析函数：解析HTTP请求
HTTP_CODE http_parse(char* buffer, int read_idx, HttpRequest& req);

#endif