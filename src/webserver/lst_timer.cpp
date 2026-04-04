#include "lst_timer.h"
#include <unistd.h>
#include <sys/epoll.h>

extern int epollfd;

sort_timer_lst::sort_timer_lst(){
    head = nullptr;
    tail = nullptr;
}
sort_timer_lst::~sort_timer_lst(){
    util_timer* tmp = head;
    while(tmp){
        head = head->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer* timer){      // 添加定时器
    if(!timer) return;
    if(!head){
        head = tail = timer;
        return;
    }

    //如果新的定时器超时时间小于当前头部结点
    //直接将当前定时器结点作为头部结点
    if(timer->expire < head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    
    //否则调用私有成员，调整内部结点
    add_timer(timer, head);
}

//私有成员，被公有成员add_timer和adjust_time调用
//主要用于调整链表内部结点
void sort_timer_lst::add_timer(util_timer* timer, util_timer* lst_head){
    util_timer* prev = lst_head;
    util_timer* tmp = prev->next;

    while(tmp){
        if(timer->expire < tmp->expire){
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }

    //遍历完发现，目标定时器需要放到尾结点处
    if(!tmp){
        prev->next = timer;
        timer->prev = prev;
        timer->next = nullptr;
        tail = timer;
    }
}

void sort_timer_lst::adjust_timer(util_timer* timer){       // 刷新定时器（活跃时）调整定时器在链表中的位置
    if(!timer) return;

    util_timer* tmp = timer->next;

    //被调整的定时器在链表尾部
    //或者 定时器超时值仍然小于下一个定时器超时值，不调整
    if(!tmp || timer->expire < tmp->expire) return;

    //被调整定时器是链表头结点，将定时器取出，重新插入
    if(timer == head){
        head = tmp;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    }
    else{   //被调整定时器在内部，将定时器取出，重新插入
        tmp->prev = timer->prev;
        timer->prev->next = tmp;
    }
}

void sort_timer_lst::del_timer(util_timer* timer){      // 删除定时器
    if(!timer) return;

    //链表中只有一个定时器，需要删除该定时器
    if((timer == head) && (timer == tail)){
        delete timer;
        head = nullptr;
        tail = nullptr;
        return;
    }

    //被删除的定时器为头结点
    if(timer == head){
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return;
    }

    //被删除的定时器为尾结点
    if(timer == tail){
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }

    //被删除的定时器在链表内部，常规链表结点删除
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;

}

void sort_timer_lst::tick(){            // 定时检查：关闭超时连接
    if(!head) return;

    //获取当前时间
    time_t cur = time(NULL);

    //遍历定时器链表
    util_timer* tmp = head;
    while(tmp){

        //链表容器为升序排列
        //当前时间小于定时器的超时时间，后面的定时器也没有到期
        if(cur < tmp->expire) break;

        //当前定时器到期，则调用回调函数，执行定时事件
        tmp->cb_func(tmp->user_data);

        //将处理后的定时器从链表容器中删除，并重置头结点
        head = tmp->next;
        if(head) head->prev = nullptr;
        delete tmp;
        tmp = head;
    }
}                            


