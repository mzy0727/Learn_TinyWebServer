#ifndef LOCKER_H
#define LOCKER_H

#include <exception>    // std异常
#include <pthread.h>    // 线程接口，提供互斥锁，条件变量
#include <semaphore.h>  // 信号量

class semaphore{
public:
    // semaphore() 是默认构造函数，用于创建一个初始值为 0 的信号量
    semaphore(){
        if(sem_init(&m_sem,0,0) != 0){
            throw std::exception();
        }
    }
    // semaphore(int num) 是带参数的构造函数，用于创建一个初始值为 num 的信号量。
    semaphore(int num){
        if(sem_init(&m_sem,0,num) != 0){
            throw std::exception();
        }
    }
    // 析构函数负责释放信号量占用的资源，调用 sem_destroy 函数
    ~semaphore(){
        sem_destroy(&m_sem);
    }
    // 内部调用了 sem_wait 函数，该函数会使当前线程阻塞，直到信号量的值大于等于 1，然后将信号量的值减少
    bool wait(){
        return sem_wait(&m_sem) == 0;
    }
    // 内部调用了 sem_post 函数，该函数将信号量的值增加
    bool post(){
        return sem_post(&m_sem)  == 0;
    }
    
private:
    sem_t m_sem;
};

class mutexlocker{
public:
    // 构造函数负责初始化互斥锁，使用 pthread_mutex_init 函数
    mutexlocker(){
        if(pthread_mutex_init(&m_mutex,NULL) != 0){
            throw std::exception();
        }
    }

    ~mutexlocker(){
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock(){
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    // get 函数返回指向互斥锁的指针，允许用户直接操作互斥锁
    pthread_mutex_t* get(){
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

class condvar{
public:
    condvar(){
        if(pthread_cond_init(&m_cond,NULL) != 0){
            throw std::exception();
        }
    }

    ~condvar(){
        pthread_cond_destroy(&m_cond);
    }
    // wait 函数用于将调用线程放入条件变量的请求队列，并解锁与给定互斥锁关联的临界区。
    // 线程将被阻塞，直到其他某个线程调用 signal 或 broadcast 来唤醒它
    bool wait(pthread_mutex_t *m_mutex){
        return pthread_cond_wait(&m_cond,m_mutex) == 0;
    }
    // timewait 函数类似于 wait，但是可以设置超时时间，如果超过指定时间条件还未满足，线程将被唤醒
    bool timewait(pthread_mutex_t * m_mutex, struct timespec t){
        return pthread_cond_timedwait(&m_cond,&m_mutex,&t) == 0;
    }
    // signal 函数用于唤醒等待在条件变量上的一个线程。如果有多个线程在等待，只有其中一个会被唤醒
    bool signal(){
        return pthread_cond_signal(&m_cond) == 0;
    }
    // broadcast 函数用于唤醒所有等待在条件变量上的线程
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond) == 0;
    }
private:
    pthread_cond_t m_cond;

};
#endif