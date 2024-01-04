# 从零实现WebServer之线程池类

## 流程图

![线程池](https://glf-1309623969.cos.ap-shanghai.myqcloud.com/img/%E7%BA%BF%E7%A8%8B%E6%B1%A0.svg)

## 构造函数 threadpool

```c++
template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPoll, uint32_t thread_number, uint32_t max_request)
    : m_actor_model(actor_model), m_connPool(connPoll), m_thread_number(thread_number), m_max_request(max_request) {
    if (thread_number <= 0 || max_request <= 0) {
        throw std::exception();
    }
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) {
        throw std::exception();
    }
    for (uint32_t i = 0; i < thread_number; i++) {
        // 内存单元、线程属性(NULL)、工作函数，传递参数（线程池）
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
        // 分离主线程与子线程，子线程结束后资源自动回收
        if (pthread_detach(m_threads[i]) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}
```

在 `threadpool` 类的构造函数 `threadpool::threadpool` 中，进行了线程池的初始化工作。下面是对该函数的一些主要步骤的解释：

1. **成员初始化列表：** 使用成员初始化列表初始化线程池的各个成员变量。

   ```c++
   template <typename T>
   threadpool<T>::threadpool(int actor_model, connection_pool *connPool, uint32_t thread_number, uint32_t max_request)
       : m_actor_model(actor_model), m_connPool(connPool), m_thread_number(thread_number), m_max_request(max_request) {
   ```

   在初始化列表中，完成对成员变量 `m_actor_model`、`m_connPool`、`m_thread_number`、`m_max_request` 的初始化。

2. **线程标识符内存分配：** 使用 `new` 运算符分配存储线程标识符的内存。

   ```c++
   m_threads = new pthread_t[m_thread_number];
   ```

   如果内存分配失败，抛出异常。

3. **创建工作线程：** 使用 `pthread_create` 函数创建线程，将线程的入口函数设置为 `worker`。

   ```c++
   for (uint32_t i = 0; i < thread_number; i++) {
       if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
           delete[] m_threads;
           throw std::exception();
       }
       // 分离主线程与子线程，子线程结束后资源自动回收
       if (pthread_detach(m_threads[i]) != 0) {
           delete[] m_threads;
           throw std::exception();
       }
   }
   ```

   循环创建工作线程，将线程的入口函数设置为 `worker`，并传递当前线程池对象作为参数。同时，使用 `pthread_detach` 将子线程分离，使其在结束后自动释放资源。

4. **异常检查：** 在函数执行过程中，进行异常检查，如果发生异常，释放已分配的资源。

   ```
   if (!m_threads) {
       throw std::exception();
   }
   ```

   在最后，释放线程标识符的内存。

该构造函数完成了线程池的初始化工作，包括成员变量的初始化、线程标识符的内存分配、工作线程的创建等。

## 析构函数 ~threadpool

```c++
// 析构，释放线程数组
template <typename T>
threadpool<T>::~threadpool(){
    delete[] m_threads;
}
```

在 `threadpool` 类的析构函数 `threadpool::~threadpool` 中，进行了线程池的资源释放工作。

**释放线程标识符数组内存：** 使用 `delete[]` 运算符释放动态分配的线程标识符数组内存。

```
delete[] m_threads;
```

该步骤的目标是在线程池对象销毁时释放动态分配的线程数组内存，以防止内存泄漏。

## 添加请求 append

```c++
// 添加请求，并利用信号量通知工作线程
template <typename T>
bool threadpool<T>::append(T *request,int state){
    // 任务队列是临界区，加互斥锁
    m_queuelocker.lock();
    if(m_workqueue.size() >= m_max_request){
        m_queuelocker.unlock();
        return false;
    }
    // 加入请求队列，然后解锁
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
     // 增加信号量，表示有任务要处理
    m_queuestat.post();
    return true;
}
```

在 `threadpool` 类的成员函数 `threadpool::append` 中，实现了向任务队列添加请求的操作。下面是对该函数的主要步骤的解释：

1. **加锁：** 使用互斥锁 `m_queuelocker` 对任务队列进行加锁，确保多个线程访问任务队列时的互斥性。

   ```c++
   m_queuelocker.lock();
   ```

2. **检查任务队列是否已满：** 判断当前任务队列的大小是否已经达到了最大请求数量 `m_max_request`。

   ```c++
   if (m_workqueue.size() >= m_max_request) {
       m_queuelocker.unlock();
       return false;
   }
   ```

   如果队列已满，释放互斥锁并返回 `false`，表示添加请求失败。

3. **设置请求状态：** 将传入的请求对象的状态设置为函数参数 `state`。

   ```c++
   request->m_state = state;
   ```

4. **加入请求队列：** 将请求对象加入到任务队列中。

   ```c++
   m_workqueue.push_back(request);
   ```

5. **解锁：** 释放互斥锁，允许其他线程访问任务队列。

   ```c++
   m_queuelocker.unlock();
   ```

6. **返回添加请求成功：** 返回 `true`，表示成功添加请求。

该函数的目标是向任务队列添加请求，并在保证线程安全的情况下进行操作。

## 添加请求,不带状态

```c++
// 添加请求，不带状态
template <typename T>
bool threadpool<T>::append_p(T *request){
    m_queuelocker.lock();
    if(m_workqueue.size() >= m_max_request){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
```

在 `threadpool` 类的成员函数 `threadpool::append_p` 中，实现了向任务队列添加请求的操作，不带状态。以下是该函数的主要步骤解释：

1. **加锁：** 使用互斥锁 `m_queuelocker` 对任务队列进行加锁，确保多个线程访问任务队列时的互斥性。

   ```c++
   m_queuelocker.lock();
   ```

2. **检查任务队列是否已满：** 判断当前任务队列的大小是否已经达到了最大请求数量 `m_max_request`。

   ```c++
   if (m_workqueue.size() >= m_max_request) {
       m_queuelocker.unlock();
       return false;
   }
   ```

   如果队列已满，释放互斥锁并返回 `false`，表示添加请求失败。

3. **加入请求队列：** 将请求对象加入到任务队列中。

   ```c++
   m_workqueue.push_back(request);
   ```

4. **解锁：** 释放互斥锁，允许其他线程访问任务队列。

   ```c++
   m_queuelocker.unlock();
   ```

5. **通知工作线程：** 使用信号量 `m_queuestat` 发送信号，通知工作线程有新的请求可以处理。

   ```c++
   m_queuestat.post();
   ```

6. **返回添加请求成功：** 返回 `true`，表示成功添加请求。

该函数的目标是向任务队列添加请求，并在保证线程安全的情况下进行操作。

## 工作线程 worker

```c++
// 工作线程运行
template <typename T>
void* threadpool<T>::worker(void *arg){
    // 转换为线程池类
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return nullptr;
}
```

在 `threadpool` 类的静态成员函数 `threadpool::worker` 中，实现了工作线程的运行。以下是该函数的主要步骤解释：

1. **类型转换：** 将传入的 `void*` 类型参数 `arg` 转换为 `threadpool*` 类型，以便访问线程池对象的成员函数。

   ```c++
   threadpool *pool = (threadpool *)arg;
   ```

2. **调用线程池的 `run` 函数：** 调用线程池对象的 `run` 函数，开始执行工作线程的任务。

   ```c++
   pool->run();
   ```

3. **返回空指针：** 由于 `pthread_create` 函数要求线程函数的返回类型为 `void*`，因此返回 `nullptr`。

   ```c++
   return nullptr;
   ```

该静态成员函数的目标是作为线程函数，在工作线程中调用线程池对象的 `run` 函数，执行具体的任务。

## 工作函数 run

```c++
// 工作函数
template <typename T>
void threadpool<T>::run(){
    while(true){
        // 信号量阻塞，等待任务
        m_queuestat.wait();
        // 请求队列临界区，加互斥锁
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        // 从请求队列取第一个请求
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request)
            continue;
        // reactor
        if(m_actor_model == 1){
            // 读
            if(request->m_state == 0){
                if(request->read_once()){
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                }
                else{
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            // 写
            else{
                if(request->write()){
                    request->improv = 1;
                }
                else{
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        // proactor
        else{
            // 从连接池中取出一个数据库连接 RAII
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            // 处理请求
            request->process();
        }
    }
}
```

在 `threadpool` 类的成员函数 `threadpool::run` 中，实现了工作线程的具体任务执行。以下是该函数的主要步骤解释：

1. **信号量等待：** 使用信号量 `m_queuestat` 进行阻塞等待，等待任务的到来。

   ```c++
   m_queuestat.wait();
   ```

2. **请求队列操作：** 进入临界区，加互斥锁，检查请求队列是否为空。

   ```c++
   m_queuelocker.lock();
   if(m_workqueue.empty()){
       m_queuelocker.unlock();
       continue;
   }
   ```

3. **取出请求：** 如果请求队列非空，从队列头取出第一个请求，并在请求队列中移除。

   ```c++
   T *request = m_workqueue.front();
   m_workqueue.pop_front();
   m_queuelocker.unlock();
   ```

4. **处理请求：** 检查请求是否为空，如果为空则继续循环。根据线程池的处理模式（`reactor `或 `proactor`）执行不同的处理逻辑。

   - Reactor 模式：
     - 如果请求是读操作（`m_state == 0`），调用 `read_once` 函数执行一次读操作，然后创建数据库连接 `connectionRAII` 对象，执行请求的处理函数 `process`。
     - 如果请求是写操作，调用 `write` 函数执行一次写操作。

   ```c++
   if(!request)
       continue;
   if(m_actor_model == 1){
       if(request->m_state == 0){
           // 读操作
           if(request->read_once()){
               request->improv = 1;
               connectionRAII mysqlcon(&request->mysql, m_connPool);
               request->process();
           }
           else{
               request->improv = 1;
               request->timer_flag = 1;
           }
       }
       // 写操作
       else{
           if(request->write()){
               request->improv = 1;
           }
           else{
               request->improv = 1;
               request->timer_flag = 1;
           }
       }
   }
   ```

   - Proactor 模式：
     - 创建数据库连接 `connectionRAII` 对象，执行请求的处理函数 `process`。

   ```c++
   else{
       // Proactor 模式
       connectionRAII mysqlcon(&request->mysql, m_connPool);
       request->process();
   }
   ```

5. **循环继续：** 任务处理完成后，继续循环，等待下一个任务的到来。

该函数的目标是作为工作线程的执行函数，不断从请求队列中取出请求并执行相应的操作。具体的操作依赖于线程池的处理模式（`Reactor` 或 `Proactor`）。

## 其他线程池

[手写线程池 | Mzy's Blog (dybil.top)](https://dybil.top/2023/11/08/手写线程池/)

