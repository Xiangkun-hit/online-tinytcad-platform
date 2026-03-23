#include "threadpool.h"

// 构造函数：指定线程数量
ThreadPool::ThreadPool(int thread_num) :m_stop(false){   
    // 创建指定数量的工作线程
    for(int i = 0; i < thread_num; ++i){
        m_threads.emplace_back(&ThreadPool::worker,this);
        std::cout << "线程 " << i + 1 << " 启动成功" << std::endl;
    }
} 

// 线程工作函数（消费者）
void ThreadPool::worker(){   
    while(true){
        std::function<void()> task;

        {
            // 加锁：操作任务队列
            std::unique_lock<std::mutex> lock(m_mutex);
            // 等待：有任务 或 线程池停止
            m_cond.wait(lock, [this]{
                return m_stop || !m_task_queue.empty();
            });

            // 线程池停止且无任务，退出
            if(m_stop && m_task_queue.empty()){
                return;
            }

            // 取出队首任务
            task = std::move(m_task_queue.front());
            m_task_queue.pop();
        }

        // 执行任务（解锁后执行，不阻塞其他线程）
        if(task){
            task();
        }

    }
} 


// // 添加任务到线程池 生产者（核心接口）
// template<class F, class... Args>
// void ThreadPool::addTask(F&& f, Args&&... args) {
//     std::function<void()> task =
//         std::bind(std::forward<F>(f),std::forward<Args>(args)...);
//     {
//         std::unique_lock<std::mutex> lock(m_mutex);
//         m_task_queue.emplace(std::move(task));
//     }
//     m_cond.notify_one(); // 唤醒一个等待的线程
// } 


// 析构函数：回收线程
ThreadPool::~ThreadPool(){   
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cond.notify_all();  // 唤醒所有线程
    // 等待所有线程执行完毕
    for(auto& thread : m_threads){
        if(thread.joinable()){
            thread.join();
        }
    }
    std::cout << "线程池已销毁" << std::endl;
} 