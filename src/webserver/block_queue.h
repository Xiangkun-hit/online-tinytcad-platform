#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <sys/time.h>

#include "locker.h"
#include "cond.h"

// 模板类阻塞队列：支持任意类型元素（后续日志用string类型）
// 循环数组实现，相比链表更省内存，访问速度更快
template <class T>
class block_queue{
public:
    // 构造函数：初始化队列最大容量
    // 入参：max_size 队列最大元素个数，默认1000
    block_queue(int max_size = 1000)
    {
        if(max_size <= 0){    //容量校验
            exit(-1);
        }
        m_max_size = max_size;
        m_array = new T[max_size];      // 初始化循环数组，存储队列元素
        m_size = 0;                     // 当前队列元素个数，初始0
        m_front = -1;                   // 队首指针，初始-1（无元素）
        m_back = -1;                    // 队尾指针，初始-1（无元素）
    }

    // 析构函数：释放内存，保证线程安全
    ~block_queue()
    {
        m_mutex.lock();
        if(m_array != nullptr){
            delete[]m_array;
            m_array = nullptr;
        }
        m_mutex.unlock();
    }

     // 接口1：清空队列
    void clear()
    {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    // 接口2：判断队列是否满
    // 返回值：true=满，false=未满
    bool full()
    {
        m_mutex.lock();
        bool res = (m_size >= m_max_size);
        m_mutex.unlock();
        return res;
    }

    // 接口3：判断队列是否空
    // 返回值：true=空，false=非空
    bool empty()
    {
        m_mutex.lock();
        bool res = (m_size == 0);
        m_mutex.unlock();
        return res;
    }

    // 接口4：获取队首元素（不出队）
    // 入参：value 存储队首元素的变量
    // 返回值：true=成功（队列非空），false=失败（队列空）
    bool front(T& value)
    {
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    // 接口5：获取队尾元素（不出队）
    // 入参：value 存储队尾元素的变量
    // 返回值：true=成功（队列非空），false=失败（队列空）
    bool back(T& value)
    {
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }

    // 接口6：获取当前队列元素个数
    int size()
    {
        m_mutex.lock();
        int tmp = m_size;
        m_mutex.unlock();
        return tmp;
    }

    // 接口7：获取队列最大容量
    int max_size()
    {
        m_mutex.lock();
        int tmp = m_max_size;
        m_mutex.unlock();
        return tmp;
    }

    // 接口8：入队（生产者）
    // 入参：item 要入队的元素
    // 返回值：true=成功，false=失败（队列满）
    // 功能：队列未满则入队，入队后唤醒所有消费者
    bool push(const T& item)
    {
        m_mutex.lock();
        // 队列满，唤醒消费者并返回失败
        if(m_size >= m_max_size){
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        // 循环数组：队尾指针后移（取模实现循环）
        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;
        m_size++;
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }

    // 接口9：出队（消费者，无限阻塞）
    // 入参：item 存储出队元素的变量
    // 返回值：true=成功，false=失败（等待被中断）
    // 功能：队列空则无限阻塞，直到被唤醒后出队
    bool pop(T& item)
    {
        m_mutex.lock();
        // 循环等待：避免C++条件变量的「虚假唤醒」
        while(m_size <= 0){
            // 等待被唤醒，失败则解锁并返回
            // if(!m_cond.wait(m_mutex.get())){
            //     m_mutex.unlock();
            //     return false;
            // }
            m_cond.wait(m_mutex.get());
        }
        // 循环数组：队首指针后移（取模实现循环）
        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

    // 接口10：出队（消费者，超时阻塞）
    // 入参1：item 存储出队元素的变量
    // 入参2：ms_timeout 超时时间（毫秒）
    // 返回值：true=成功，false=失败（超时/等待被中断）
    // 功能：队列空则阻塞指定毫秒，超时或被唤醒后出队
    bool pop(T& item, int ms_timeout)
    {
        struct timespec t = {0,0};
        struct timeval now = {0,0};
        gettimeofday(&now, NULL);// 获取当前系统时间
        m_mutex.lock();
        if(m_size <= 0){
            // 计算超时时间点：当前时间 + 超时毫秒数
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            // 超时等待，失败则解锁并返回
            if(!m_cond.timewait(m_mutex.get(),t)){
                m_mutex.unlock();
                return false;
            }
        }
        // 超时后仍无元素，返回失败
        if(m_size <= 0){
            m_mutex.unlock();
            return false;
        }
        // 正常出队，逻辑同无限阻塞版
        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }





private:
    locker m_mutex;         // 队列互斥锁：保护所有成员变量的线程安全
    cond m_cond;            // 队列条件变量：实现生产者-消费者的等待/唤醒
    T* m_array;             // 循环数组：存储队列元素
    int m_size;             // 当前队列元素个数
    int m_max_size;         // 队列最大容量
    int m_front;            // 队首指针：指向当前队首元素的下标
    int m_back;             // 队尾指针：指向当前队尾元素的下标
};




#endif