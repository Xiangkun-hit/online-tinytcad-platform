//封装 C++11 原生std::mutex，
//为后续日志文件写入、阻塞队列操作提供线程安全的互斥锁，
//对外提供简洁的lock()/unlock()/get()接口，隐藏底层实现细节。
#ifndef LOCKER_H
#define LOCKER_H

#include <mutex>

// 互斥锁封装类：线程安全，适配日志/阻塞队列的锁操作
// 所有成员函数内联实现，轻量无性能损耗
class locker{
public:
    std::mutex& get(){ return m_mutex;}   // 核心接口1：获取原生互斥锁（给条件变量cond类的wait/timewait函数使用）
    void lock(){ m_mutex.lock(); }        // 核心接口2：加锁（获取互斥锁，其他线程需等待）
    void unlock(){m_mutex.unlock();}      // 核心接口3：解锁（释放互斥锁，唤醒等待的线程）

private:
    std::mutex m_mutex;    // 私有成员：C++11原生互斥锁，禁止外部直接访问
};


#endif