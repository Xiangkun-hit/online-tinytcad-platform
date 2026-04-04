#ifndef LST_TIMER_H
#define LST_TIMER_H

#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include "locker.h"

class util_timer;

// 客户端数据：绑定socket和定时器
struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer* timer;
};

// 定时器节点
class util_timer{
public:
    util_timer():prev(nullptr), next(nullptr) {}
public:
    time_t expire;      // 超时时间（绝对时间）
    void (*cb_func)(client_data*);      // 超时回调：关闭连接
    client_data* user_data;             // 绑定的客户端
    util_timer* prev;                   //前向定时器
    util_timer* next;                   //后继定时器
};

// 升序链表定时器容器
class sort_timer_lst{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer* timer);      // 添加定时器
    void adjust_timer(util_timer* timer);   // 刷新定时器（活跃时）
    void del_timer(util_timer* timer);      // 删除定时器
    void tick();                            // 定时检查：关闭超时连接

private:
    void add_timer(util_timer* timer, util_timer* lst_head);
    util_timer* head;                       
    util_timer* tail;                       
};

// void (*cb_func)(client_data* user_data);

#endif