#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

// 线程池类
class ThreadPool{
public:
    ThreadPool(int thread_num);  // 构造函数：指定线程数量
    ~ThreadPool(); // 析构函数：回收线程

    template<class F, class... Args>
    void addTask(F&& f, Args&&... args);   // 添加任务到线程池 生产者（核心接口）
    
    // void addTask(std::function<void()> task);  

private:
    void worker();  // 线程工作函数（消费者）

private:
    std::vector<std::thread> m_threads;  // 线程池数组
    std::queue<std::function<void()>> m_task_queue;  // 任务队列

    std::mutex m_mutex;  // 互斥锁：保护任务队列
    std::condition_variable m_cond;  // 条件变量：唤醒等待的线程

    bool m_stop;  // 线程池退出标志
};



#endif