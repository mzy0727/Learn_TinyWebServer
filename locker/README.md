# 从零实现WebServer之线程同步机制包装类



## 信号量封装类

{% note success %}

**初始化信号量**

int sem_init(sem_t *sem, int pshared, unsigned int value);

  sem 信号量对象

  pshared 不为0信号量在进程间共享，否则在当前进程的所有线程共享

  value 信号量值大小

  成功返回0，失败返回-1并设置errno



**原子操作V**

int sem_wait(sem_t *sem);

  信号量不为0则-1，为0则阻塞

  成功返回0，失败返回-1并设置errno



**原子操作P**

int sem_post(sem_t *sem);

  信号量+1

  成功返回0，失败返回-1并设置errno



**销毁信号量**

int sem_destroy(sem_t *sem);

  成功返回0，失败返回-1并设置errno

{% endnote %}



```c++
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
```

这段代码定义了一个名为`semaphore`的类，它实现了一个信号量（Semaphore）。信号量是一种用于多线程或多进程之间同步的机制，它可以用来控制对共享资源的访问。

1. **构造函数 `semaphore()` 和 `semaphore(int num)`：**
   - `semaphore()` 是默认构造函数，用于创建一个初始值为 0 的信号量。
   - `semaphore(int num)` 是带参数的构造函数，用于创建一个初始值为 `num` 的信号量。
   - 在两个构造函数中，都调用了 `sem_init` 函数来初始化信号量。如果初始化失败（`sem_init` 返回非零值），则抛出 `std::exception` 异常。
2. **析构函数 `~semaphore()`：**
   - 析构函数负责释放信号量占用的资源，调用 `sem_destroy` 函数。
3. **成员函数 `bool wait()`：**
   - `wait` 函数用于等待信号量的值变为大于等于 1。如果调用成功，它返回 `true`；否则，返回 `false`。
   - 内部调用了 `sem_wait` 函数，该函数会使当前线程阻塞，直到信号量的值大于等于 1，然后将信号量的值减少。
   - `sem_wait` 的作用是使调用线程阻塞，直到信号量的值大于等于 1。当信号量的值大于等于 1 时，`sem_wait` 将信号量的值减一，并立即返回。如果信号量的值已经是 0，那么 `sem_wait` 将阻塞调用线程，直到有其他线程调用了 `sem_post` 增加了信号量的值。
4. **成员函数 `bool post()`：**
   - `post` 函数用于增加信号量的值，表示资源可用。如果调用成功，它返回 `true`；否则，返回 `false`。
   - 内部调用了 `sem_post` 函数，该函数将信号量的值增加。
5. **私有成员 `sem_t m_sem`：**
   - `m_sem` 是一个 `sem_t` 类型的信号量变量，用于存储信号量的状态信息。

这个类的目的是提供一个简单的接口，允许线程之间进行同步，通过 `wait` 和 `post` 函数来控制对共享资源的访问。使用信号量的一个常见场景是确保在多线程环境中对临界区（critical section）的互斥访问。

## 互斥锁封装类

{% note success %}

**初始化互斥锁**

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);

  mutex 互斥锁对象

  attr 互斥锁属性，默认普通锁

  value信号量值大小

  成功返回0



**加锁**

int pthread_mutex_lock (pthread_mutex_t *mutex);

  请求锁线程形成等待序列，解锁后按优先级获得锁

  成功返回0



**解锁**

int pthread_mutex_unlock (pthread_mutex_t *mutex);

  成功返回0



**销毁互斥锁**

int pthread_mutex_destroy (pthread_mutex_t *mutex);

  成功返回0

{% endnote %}

```c++
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
```

这段代码定义了一个名为 `mutexlocker` 的类，用于实现互斥锁（Mutex）。互斥锁是一种用于多线程之间同步的机制，它确保同时只有一个线程可以访问被保护的资源。

1. **构造函数 `mutexlocker()`：**
   - 构造函数负责初始化互斥锁，使用 `pthread_mutex_init` 函数。
   - 如果初始化失败，即 `pthread_mutex_init` 返回非零值，那么抛出 `std::exception` 异常。
2. **析构函数 `~mutexlocker()`：**
   - 析构函数负责销毁互斥锁，使用 `pthread_mutex_destroy` 函数。
3. **成员函数 `bool lock()`：**
   - `lock` 函数用于获取互斥锁。如果调用成功，它返回 `true`；否则，返回 `false`。
   - 内部调用了 `pthread_mutex_lock` 函数，该函数会阻塞调用线程，直到成功获取互斥锁。
4. **成员函数 `bool unlock()`：**
   - `unlock` 函数用于释放互斥锁。如果调用成功，它返回 `true`；否则，返回 `false`。
   - 内部调用了 `pthread_mutex_unlock` 函数，该函数会释放互斥锁。
5. **成员函数 `pthread_mutex_t\* get()`：**
   - `get` 函数返回指向互斥锁的指针，允许用户直接操作互斥锁。
6. **私有成员 `pthread_mutex_t m_mutex`：**
   - `m_mutex` 是一个 `pthread_mutex_t` 类型的互斥锁变量，用于存储互斥锁的状态信息。

这个类的目的是提供一个简单的接口，允许线程对临界区进行互斥访问。通过 `lock` 和 `unlock` 函数，以及 `get` 函数，用户可以方便地操作互斥锁，确保在多线程环境中对共享资源的安全访问。在构造函数和析构函数中进行初始化和销毁操作，确保互斥锁的正确使用。

## 条件变量封装类

{% note success %}

**初始化条件变量**

int pthread_cond_init(pthread_cond_t *cv, const pthread_condattr_t *cattr);

  cv 条件变量对象

  cattr 条件变量属性

  成功返回0



**等待条件变量成立**

int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mutex);

  调用该函数时，线程总处于某个临界区，持有某个互斥锁

  释放mutex防止死锁，阻塞等待唤醒，然后再获取mutex

  成功返回0



**计时等待条件变量成立**

int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *mutex, struct timespec *abstime);

  成功返回0，超时返回ETIMEDOUT



**唤醒一个wait的线程**

int pthread_cond_signal(pthread_cond_t *cv);

  成功返回0



**唤醒所有wait的线程**

int pthread_cond_broadcast(pthread_cond_t *cv);

  成功返回0

{% endnote %}

```c++
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
```

这段代码定义了一个名为 `condvar` 的类，用于实现条件变量（Condition Variable）。条件变量是一种线程同步的机制，它允许线程在某个条件成立之前等待，并在条件成立时被唤醒。

1. **构造函数 `condvar()`：**
   - 构造函数负责初始化条件变量，使用 `pthread_cond_init` 函数。
   - 如果初始化失败，即 `pthread_cond_init` 返回非零值，那么抛出 `std::exception` 异常。
2. **析构函数 `~condvar()`：**
   - 析构函数负责销毁条件变量，使用 `pthread_cond_destroy` 函数。
3. **成员函数 `bool wait(pthread_mutex_t \*m_mutex)`：**
   - `wait` 函数用于将调用线程放入条件变量的请求队列，并解锁与给定互斥锁关联的临界区。线程将被阻塞，直到其他某个线程调用 `signal` 或 `broadcast` 来唤醒它。
   - 返回值为 `true` 表示成功，`false` 表示出错。
4. **成员函数 `bool timewait(pthread_mutex_t \*m_mutex, struct timespec t)`：**
   - `timewait` 函数类似于 `wait`，但是可以设置超时时间，如果超过指定时间条件还未满足，线程将被唤醒。
   - 返回值为 `true` 表示成功，`false` 表示出错。
5. **成员函数 `bool signal()`：**
   - `signal` 函数用于唤醒等待在条件变量上的一个线程。如果有多个线程在等待，只有其中一个会被唤醒。
   - 返回值为 `true` 表示成功，`false` 表示出错。
6. **成员函数 `bool broadcast()`：**
   - `broadcast` 函数用于唤醒所有等待在条件变量上的线程。
   - 返回值为 `true` 表示成功，`false` 表示出错。
7. **私有成员 `pthread_cond_t m_cond`：**
   - `m_cond` 是一个 `pthread_cond_t` 类型的条件变量变量，用于存储条件变量的状态信息。

这个类的目的是提供一个简单的接口，允许线程等待某个条件的发生，并在条件满足时被唤醒。通过 `wait`、`timewait`、`signal` 和 `broadcast` 函数，以及构造函数和析构函数的初始化和销毁操作，用户可以方便地使用条件变量来实现线程之间的同步。