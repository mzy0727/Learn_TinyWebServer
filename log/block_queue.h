#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <deque>
#include "../locker/locker.h"


using namespace std;

// 阻塞队列模板类

template <class T>
class block_queue{
    
public:
    // block_queue类的构造函数用于初始化实例
    block_queue(int max_size = 1000){
        if(max_size <= 0){
            exit(-1);
        }
        m_max_size = max_size;
        
    }
    ~block_queue(){
        clear();
    }
    // clear 函数通过互斥锁确保了对队列的操作是线程安全的
    void clear(){
        m_mutex.lock();     // 加锁，确保在多线程环境中对队列的操作是互斥的
        m_deque.clear();    // 清空队列，移除所有元素
        m_mutex.unlock();   // 解锁，释放互斥锁，允许其他线程对队列进行操作
    }

    bool full(){
        m_mutex.lock();
        if(m_deque.size() >= m_max_size){
            m_mutex.unlock();   // 如果队列已满，解锁互斥锁
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool empty(){
        m_mutex.lock();
        if(m_deque.empty())
            return true;
        m_mutex.unlock();
        return false;
    }
    bool front(T &item){
        m_mutex.lock();
        if(m_deque.empty()){
            m_mutex.unlock();
            return false;
        }
        item = m_deque.front();
        m_mutex.unlock();
        return true;
    }

    bool back(T &item){
        m_mutex.lock();
        if(m_deque.empty()){
            m_mutex.unlock();
            return false;
        }
        item = m_deque.back();
        m_mutex.unlock();
        return true;
    }
    int get_size(){
        int temp = 0;
        m_mutex.lock();
        temp = m_deque.size();
        m_mutex.unlock();
        return temp;
    }
    int get_max_size(){
        int temp = 0;
        m_mutex.lock();
        temp = m_max_size;
        m_mutex.unlock();
        return temp;
    }
    bool push_back(const T &item){
        m_mutex.lock();     // 加锁，确保在多线程环境中对队列的操作是互斥的
        if(m_deque.size() >= m_max_size){   // 判断队列大小是否达到最大允许大小
            m_cond.broadcast();     // 唤醒所有在条件变量上等待的线程
            m_mutex.unlock();   // 解锁互斥锁
            return false;       // 返回false，表示队列已满，无法添加元素
        }
        m_deque.push_back(item);    // 向队列尾部添加元素
        m_cond.broadcast(); // 唤醒所有在条件变量上等待的线程
        m_mutex.unlock();   // 解锁互斥锁
        return true;
    }

    bool pop(T &item){
        m_mutex.lock();
        while(m_deque.empty()){     // 使用循环等待，直到队列非空
            if(!m_cond.wait(m_mutex.get())){    // 如果条件变量等待失败
                m_mutex.unlock();   // 解锁互斥锁
                return false;
            }
        }
        item = m_deque.front();
        m_deque.pop_front();
        m_mutex.unlock();
        return true;
    }

    // 增加超时处理
    bool pop(T &item, int ms_timeout) {
        struct timespec t = {0, 0};  // s and ns
        struct timeval now = {0, 0}; // s and ms
        gettimeofday(&now, nullptr);

        m_mutex.lock();
        if (m_size <= 0) {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;     //使用超时参数 ms_timeout 计算出等待的超时时间
            if (!m_cond.timewait(m_mutex.get(), t)) {   // 等待超时处理
                m_mutex.unlock();
                return false;
            }
        }

        item = m_deque.front();
        m_deque.pop_front();
        m_mutex.unlock();
        return true;
    }

private:
    mutexlocker m_mutex;    // 互斥锁
    condvar m_cond;         // 条件变量，用于在队列为空或队列已满时进行等待和唤醒

    deque<T> m_deque;
    int m_max_size;
};


#endif