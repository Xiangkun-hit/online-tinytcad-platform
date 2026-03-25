#include "http_parser.h"
#include <cstring>
#include <cstdlib>
#include "log.h"

// 从状态机--------从缓冲区读取一行（HTTP标准 \r\n 换行）
LINE_STATUS parse_line(char* buffer, int& checked_idx, int& read_idx){
    for(;checked_idx < read_idx; checked_idx++){
        if(buffer[checked_idx] == '\r'){
            if(checked_idx + 1 == read_idx) return LINE_STATUS::LINE_OPEN;
            if(buffer[checked_idx + 1] == '\n'){
                buffer[checked_idx] = '\0';
                buffer[checked_idx + 1] = '\0';
                checked_idx += 2;
                return LINE_STATUS::LINE_OK;
            }
        return LINE_STATUS::LINE_BAD;  // \r下一个不是\n 返回行错误
        }
    }
    return LINE_STATUS::LINE_OPEN;
}

// 解析请求行 METHOD url
HTTP_CODE parse_request_line(char* line, CHECK_STATE& state, HttpRequest& req){
    char* method = strpbrk(line, " \t");
    if(!method) {
        LOG_ERROR("HTTP请求行解析失败:未找到方法与URL的分隔符(请求行：%s)", line);
        return HTTP_CODE::BAD_REQUEST;
    }
    *method = '\0';
    method++;

    char* url = strpbrk(method, " \t");
    if(!url) {
        LOG_ERROR("HTTP请求行解析失败:未找到URL与版本的分隔符(请求行：%s)", line);
        return HTTP_CODE::BAD_REQUEST;
    }
    *url = '\0';
    url++;

    req.method = line;
    req.url = url;
    state = CHECK_STATE::CHECK_STATE_HEADER;
    return HTTP_CODE::NO_REQUEST;
}

// 解析请求头
HTTP_CODE parse_headers(char* line, CHECK_STATE& state, HttpRequest& req){
    if(*line == '\0'){
        if(req.content_len > 0){
            state = CHECK_STATE::CHECK_STATE_CONTENT;
            return HTTP_CODE::NO_REQUEST;
        }
        return(req.method == "GET") ? HTTP_CODE::GET_REQUEST : HTTP_CODE::POST_REQUEST;
    }
    if(strncasecmp(line, "Content-Length:", 15) == 0){
        line += 15;
        req.content_len = atoi(line);
    }
    return HTTP_CODE::NO_REQUEST;
}

// 解析请求体
HTTP_CODE parse_content(char* buffer, int& checked_idx, int read_idx, HttpRequest& req){
    if(checked_idx + req.content_len > read_idx + 1) {
        LOG_WARN("HTTP请求体解析失败:请求体不完整（预期：%d字节,实际：%d字节)",req.content_len,read_idx-checked_idx);
        return HTTP_CODE::NO_REQUEST;
    }
    req.body = buffer + checked_idx;
    checked_idx += req.content_len;
    return HTTP_CODE::POST_REQUEST;
}


// 主状态机--------状态机主函数
HTTP_CODE http_parse(char* buffer, int read_idx, HttpRequest& req){
    if(read_idx <= 0) return HTTP_CODE::NO_REQUEST;  //基础校验

    CHECK_STATE state = CHECK_STATE::CHECK_STATE_REQUEST_LINE;   //初始化核心变量 解析请求行
    LINE_STATUS line_status = LINE_STATUS::LINE_OK;              //行状态初始为完整
    HTTP_CODE ret = HTTP_CODE::NO_REQUEST;                       //解析结果
    int checked_idx = 0;                                         //已经解析到的缓冲区位置

    while((line_status = parse_line(buffer, checked_idx, read_idx)) == LINE_STATUS::LINE_OK){
        char* line = buffer + checked_idx;              // 拿到当前完整的一行

        switch(state){                      // 按当前状态处理这一行
            case CHECK_STATE::CHECK_STATE_REQUEST_LINE:
                ret = parse_request_line(line, state, req);
                break;
            case CHECK_STATE::CHECK_STATE_HEADER:
                ret = parse_headers(line, state, req);
                break;
            default:
                break;   
        }
        if(ret == HTTP_CODE::BAD_REQUEST) return HTTP_CODE::BAD_REQUEST;
    }
    if(line_status == LINE_STATUS::LINE_OPEN && state == CHECK_STATE::CHECK_STATE_CONTENT){
        ret = parse_content(buffer, checked_idx, read_idx, req);
    }
    return ret;
}
