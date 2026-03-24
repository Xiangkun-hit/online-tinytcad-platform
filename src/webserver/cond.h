#ifndef COND_H
#define COND_H

#include <condition_variable>
#include <mutex>
#include <time.h>

// 条件变量封装类：与locker配合使用，实现线程的等待/唤醒
class cond{
public:
    // 接口1：普通等待（无限阻塞）
    // 入参：std::mutex& 原生互斥锁（由locker::get()提供）
    // 功能：释放互斥锁，阻塞当前线程，直到被broadcast()唤醒
    void wait(std::mutex& mutex){
        // 封装C++11的unique_lock，适配原生条件变量的wait要求
        // std::adopt_lock：表示当前线程已持有互斥锁，无需重复加锁
        std::unique_lock<std::mutex> lock(mutex, std::adopt_lock);
        m_cond.wait(lock);    // 等待唤醒，唤醒后自动重新获取互斥锁
        lock.release();       // 释放unique_lock的管理权，避免重复解锁
        
    }

    // 接口2：广播唤醒
    // 功能：唤醒所有等待当前条件变量的线程（生产者生产后，唤醒所有消费者）
    void broadcast(){
        m_cond.notify_all();
    }
    void signal(){
        m_cond.notify_one();
    }

    // 接口3：超时等待（有限阻塞）
    // 入参1：std::mutex& 原生互斥锁
    // 入参2：const timespec& 超时时间点
    // 返回值：true=被唤醒，false=超时
    // 功能：释放互斥锁，阻塞当前线程，超时或被唤醒后返回
    bool timewait(std::mutex& mutex, const timespec& ts){
        std::unique_lock<std::mutex> lock(mutex, std::adopt_lock);
        // 将C语言的timespec转换为C++11的时间点
        auto wait_time = std::chrono::system_clock::from_time_t(ts.tv_sec) +
                         std::chrono::nanoseconds(ts.tv_nsec);
        // 等待直到超时或被唤醒，返回等待状态
        std::cv_status res = m_cond.wait_until(lock, wait_time);
        lock.release();
        // 若不是超时，则返回true（被唤醒）
        return res != std::cv_status::timeout;    
    }

private:
    std::condition_variable m_cond;  // 私有成员：C++11原生条件变量，禁止外部直接访问
};

#endif