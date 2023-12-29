# 从零实现WebServer之日志类



## 阻塞队列模板类

{% note success %}

构造函数，初始化队列的最大容量，默认为1000
block_queue(int max_size = 1000);
析构函数，清空队列
~block_queue();
清空队列
void clear();
判断队列是否已满
bool full();
判断队列是否为空
bool empty();
返回队首元素
bool front(T &item);
返回队尾元素
bool back(T &item);
获取队列当前元素个数
int get_size();
获取队列的最大容量
int get_max_size();
向队尾添加元素
bool push_back(const T &item);
从队头弹出元素
bool pop(T &item);
增加超时处理的pop函数
bool pop(T &item, int ms_timeout);

{% endnote %}

```c++
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
```

这个`block_queue`类是一个线程安全的阻塞队列实现，使用了互斥锁和条件变量来保证在多线程环境中的操作的互斥性和同步性。这个`block_queue`类适用于需要在多线程环境中进行数据共享的场景，提供了一种安全且高效的队列实现。

1. **线程安全性**：通过使用`mutexlocker`和`condvar`实现了互斥锁和条件变量，确保多个线程对队列的并发操作是安全的。
2. **阻塞操作**：在队列为空时，`pop`操作将会等待直到队列中有元素可供取出；在队列已满时，`push_back`操作将会等待直到队列中有空位可供插入。
3. **容量控制**：通过`full()`和`empty()`方法提供了对队列是否已满或为空的判断。
4. **超时处理**：`pop`操作提供了带有超时参数的版本，允许用户设定最长等待时间，避免无限等待。
5. **使用deque实现**：底层使用了`std::deque`作为队列容器，相比于`std::queue`，`std::deque`允许从队列的前端和后端高效地进行元素的添加和删除。
6. **清空队列**：提供了`clear`方法，用于清空队列并释放资源。
7. **异常处理**：在构造函数中，如果指定的最大容量小于等于0，程序将终止，以防止非法输入。

## 日志类

### 流程图

![io模块](https://glf-1309623969.cos.ap-shanghai.myqcloud.com/img/io.svg)

### 获取实例 get_instance

```c++
 // 公有静态方法，用于获取唯一的实例
static Log* get_instance(){
    // 静态局部变量，确保只在第一次调用该函数时创建一个实例
    static Log instance;
    // 返回指向实例的指针
    return &instance;
}
```

1. `get_instance` 是一个公有的静态成员函数，用于获取类的唯一实例。
2. `static Log instance;` 是一个静态局部变量，确保在程序的整个生命周期内只有一个实例。这是C++11引入的线程安全的局部静态变量。
3. `return &instance;` 返回指向唯一实例的指针。

### 构造函数 Log

```c++
private:
    Log();
    ~Log();
    // 防止复制构造和赋值操作，确保单例的唯一性
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
```

这是一个典型的单例模式实现，通过将构造函数和析构函数声明为私有的，以及禁用拷贝构造函数和赋值操作符，确保了类的单例性。这样，只能通过 `get_instance` 函数获取唯一的实例，而不能直接复制或赋值。

1. 构造函数和析构函数是私有的，防止外部直接实例化或销毁对象。
2. 拷贝构造函数和赋值操作符被删除，防止复制对象。
3. `get_instance` 函数提供了获取类唯一实例的途径，同时在需要时进行实例化。

### 异步写 async_write_log

```c++
// 异步写
    void async_write_log(){
        string single_log;
        // 从阻塞队列取出一个日志
        while(m_log_duque->pop(single_log)){
            m_mutex.lock();
            // 写入后文件指针后移
            fputs(single_log.c_str(),m_fp);
            m_mutex.unlock();
        
        }
    }
```

1. `async_write_log` 函数通过阻塞队列 `m_log_duque` 获取一个日志条目 `single_log`。
2. 通过互斥锁 `m_mutex` 对文件指针 `m_fp` 进行保护，确保在多线程环境下对文件的写入操作是互斥的。
3. 使用 `fputs(single_log.c_str(), m_fp);` 将日志写入文件。这里假设 `single_log` 中已经包含了完整的日志内容，因为它是从阻塞队列中取出的。
4. 写入完成后解锁互斥锁，允许其他线程进行文件写入。

需要注意的是，这个函数的执行是在一个独立的线程中进行的，通过异步写入的方式，避免了在主线程中直接写文件可能导致的性能问题。这种异步的方式允许主线程继续执行其他任务，而不必等待文件写入完成。

```c++
// 线程工作函数，静态函数防止this指针
static void* flush_log_thread(void *args){
    Log::get_instance()->async_write_log();
    return nullptr;
}
```

1. `flush_log_thread` 是一个静态成员函数，它被设计为一个线程的入口函数，用于执行异步写日志的操作。
2. 静态函数中调用了 `Log::get_instance()->async_write_log();`，这样就通过单例模式获取了 `Log` 类的唯一实例，然后调用了 `async_write_log` 函数，实现了异步写日志的逻辑。
3. 函数返回 `nullptr`，这是符合线程入口函数的标准，即线程执行完毕后返回的是 `nullptr`。

这段代码的目的是启动一个线程，用于在后台执行异步写日志的操作，从而不阻塞主线程的执行。在 `Log` 类的构造函数中，可以看到这个线程是在初始化时被创建的。该线程的生命周期和 `Log` 类的生命周期是一致的。当 `Log` 类的实例被销毁时，这个线程也应该被正确地终止。

### 宏定义 snprintf_nowarn

```c++
// 检查snprintf返回值，防止warning
#define snprintf_nowarn(...) (snprintf(__VA_ARGS__) < 0 ? abort() : (void)0)
// 可变参数宏__VA_ARGS__，当可变参数个数为0时，##__VA_ARGS__把前面多余的“，”去掉
#define LOG_DEBUG(format, ...) if(m_close_log == 0) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(m_close_log == 0) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(m_close_log == 0) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(m_close_log == 0) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}

```

这段代码定义了一组宏，用于在日志中输出不同级别的日志信息。下面是对每个宏的解释：

1. `snprintf_nowarn`: 这是一个宏，用于检查 `snprintf` 的返回值，防止编译器产生警告。如果 `snprintf` 返回值小于 0，就调用 `abort` 终止程序，否则不进行任何操作。
2. `LOG_DEBUG(format, ...)`: 这是一个宏，用于输出调试级别的日志信息。如果 `m_close_log` 不为 0（即日志未关闭），则调用 `Log::get_instance()->write_log(0, format, ##__VA_ARGS__)` 写入调试日志，然后调用 `Log::get_instance()->flush()` 刷新日志。
3. `LOG_INFO(format, ...)`: 类似于 `LOG_DEBUG`，用于输出信息级别的日志。
4. `LOG_WARN(format, ...)`: 类似于 `LOG_DEBUG`，用于输出警告级别的日志。
5. `LOG_ERROR(format, ...)`: 类似于 `LOG_DEBUG`，用于输出错误级别的日志。

这些宏的使用方式类似于普通的日志输出函数，但通过宏的方式可以方便地控制是否输出日志（通过 `m_close_log` 变量的值判断）。这样的设计可以在日志关闭时，避免不必要的字符串拼接和日志输出操作，提高程序性能。

### 初始化日志 init

```c++
// 初始化日志，异步需要设置阻塞队列的长度，同步不需要
bool Log::init(const char *file_name, int log_buf_size, int split_lines, int max_queue_size){
    // 如果设置了max_queue_size,则设置为异步
    if(max_queue_size >= 1){
        m_is_async = true;
        m_log_duque = new block_queue<string>(max_queue_size);
        //创建线程异步写
        pthread_t tid;
        pthread_create(&tid,nullptr,flush_log_thread,nullptr);
    }

    // 初始化日志
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf,0,m_log_buf_size);
    m_split_lines = split_lines;

    // 获取当前时间
    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    // 查找字符从右边开始第一次出现的位置，截断文件名
    const char *p = strrchr(file_name,'/');
    char log_full_name[256] = {0};

    // 构造日志文件名
    if(p == nullptr){
        // 将可变参数格式化到字符串中
        // 没有/则直接到当前路径下
        snprintf_nowarn(log_full_name,255,"%d_%02d_%02d_%s",my_tm.tm_year + 1900,my_tm.tm_mon+1,my_tm.tm_mday,file_name);
    }else{
        strcpy(log_name,p+1);   // 日志文件名
        strncpy(dir_name,file_name,p-file_name+1);  // 日志路径
        snprintf_nowarn(log_full_name,255,"%s%d_%02d_%02d_%s",dir_name,my_tm.tm_year + 1900,my_tm.tm_mon+1,my_tm.tm_mday,log_name);
    }
    m_today = my_tm.tm_mday;
    // 追加写
    m_fp = fopen(log_full_name,"a");
    if(m_fp == nullptr)
        return false;
    return true;
}
```

这段 C++ 代码定义了日志类 `Log` 中的初始化函数 `init`。此函数用于初始化日志记录，包括设置日志文件名、缓冲区大小、拆分行数等参数。如果设置了异步模式，还会创建一个线程用于异步写日志。

1. **判断是否启用异步模式：**

```c++
if (max_queue_size >= 1) {
    m_is_async = true;
    m_log_duque = new block_queue<string>(max_queue_size);
    // 创建线程异步写
    pthread_t tid;
    pthread_create(&tid, nullptr, flush_log_thread, nullptr);
}
```

如果设置了 `max_queue_size` 大于等于 1，表示启用异步模式。在异步模式下，创建了一个 `block_queue<string>` 类型的阻塞队列，并通过线程 `pthread_create` 创建了一个用于异步写日志的线程。

2. **初始化日志参数：**

```c++
m_log_buf_size = log_buf_size;
m_buf = new char[m_log_buf_size];
memset(m_buf, 0, m_log_buf_size);
m_split_lines = split_lines;
```

初始化日志缓冲区大小、拆分行数等参数。

3. **获取当前时间并构造日志文件名：**

```c++
time_t t = time(nullptr);
struct tm *sys_tm = localtime(&t);
struct tm my_tm = *sys_tm;

const char *p = strrchr(file_name, '/');
char log_full_name[256] = {0};
```

获取当前时间，并通过 `localtime` 函数转换为本地时间。根据文件名中是否包含路径分隔符 `/`，构造日志文件名 `log_full_name`。

4. **设置日志文件名和路径：**

```c++
if (p == nullptr) {
    snprintf_nowarn(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
} else {
    strcpy(log_name, p + 1);   // 日志文件名
    strncpy(dir_name, file_name, p - file_name + 1);  // 日志路径
    snprintf_nowarn(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
}
```

根据文件名中是否包含路径分隔符 `/`，设置日志文件名和路径。如果没有路径分隔符，则直接在当前路径下创建日志文件。如果文件名中包含路径分隔符，首先使用 `strcpy` 将文件名部分（不包含 `/`）拷贝到 `log_name` 中。接着，使用 `strncpy` 将路径部分拷贝到 `dir_name` 中，注意这里复制的长度为 `p - file_name + 1`，确保包含路径分隔符。最后，使用 `snprintf_nowarn` 将路径和文件名组合成完整的日志文件名。

5. **打开日志文件：**

```c++
m_fp = fopen(log_full_name, "a");
if (m_fp == nullptr)
    return false;
```

使用 `fopen` 打开日志文件，以追加写入的方式打开。如果打开失败，则返回 `false`。

### 写日志 write_log

```c++
// 写日志
void Log::write_log(int level, const char *format,...){
    // 秒、微妙
    struct timeval now = {0,0};
    gettimeofday(&now,nullptr);
    // 获取时间结构体
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    // 临界区加锁
    m_mutex.lock();

    m_count++;
    // 新的一天或者日志达到最大行数，需要更换日志文件
    if(m_today != my_tm.tm_mday || m_count % m_split_lines == 0){
        // 刷新文件缓冲并关闭文件
        fflush(m_fp);
        fclose(m_fp);

        // 新日志路径
        char new_log[256] = {0};
        char tail[16] = {0};

        snprintf_nowarn(tail,16,"%d_%02d_%02d_",my_tm.tm_year + 1900,my_tm.tm_mon+1,my_tm.tm_mday);

        // 天数变化
        if(m_today != my_tm.tm_mday){
            snprintf_nowarn(new_log,255,"%s%s%s",dir_name,tail,log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }else{  // 日志达到最大行数
            snprintf_nowarn(new_log,255,"%s%s%s.%lld",dir_name,tail,log_name,m_count / m_split_lines);
        }
        // 打开新日志
        m_fp = fopen(new_log,"a");       
    }
    m_mutex.unlock();

    // 日志级别
    char s[16] = {0};
    switch(level){
        case 0: 
            strcpy(s,"[debug]:");
            break;
        case 1:
            strcpy(s,"[info]:");
            break;
        case 2:
            strcpy(s,"[warn]:");
            break;
        case 3:
            strcpy(s,"[error]:");
            break;
        default:
           strcpy(s,"[debug]:");
            break; 
    }
    // 临界区加锁
    m_mutex.lock();
    // 构造日志内容、时间、级别
    int n = snprintf(m_buf,48,"%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                            my_tm.tm_year+1900,my_tm.tm_mon+1,my_tm.tm_mday,
                            my_tm.tm_hour,my_tm.tm_min,my_tm.tm_sec,now.tv_usec,s);
    if(n < 0){
        abort();
    }
    // 正文
    va_list valst;
    va_start(valst,format);
    int m = vsnprintf(m_buf + n,m_log_buf_size -1, format, valst);
    if(m < 0){
        abort();
    }
    va_end(valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';

    // 异步写
    if(m_is_async && !m_log_duque->full()){
        m_log_duque->push_back(m_buf);
    }else{  // 同步写
        fputs(m_buf,m_fp);
    }
    m_mutex.unlock();
}
```

这段代码是 `Log` 类的写日志函数 `write_log`，它实现了日志的记录和切割功能。以下是对代码的详细解释：

1. **获取当前时间：**

   ```c++
   struct timeval now = {0,0};
   gettimeofday(&now,nullptr);
   time_t t = now.tv_sec;
   struct tm *sys_tm = localtime(&t);
   struct tm my_tm = *sys_tm;
   ```

   使用 `gettimeofday` 获取当前时间（秒和微秒），然后将秒数转换为本地时间结构体 `tm`，保存在 `my_tm` 中。

2. **判断是否需要切换日志文件：**

   ```c++
   m_mutex.lock();
   m_count++;
   if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {
   ```

   每次写入日志，都会判断当前日期是否与 `m_today` 相同，以及是否达到了日志的最大行数。如果是，则需要切换到新的日志文件。

3. **关闭并刷新当前日志文件，打开新日志文件：**

   ```c++
   fflush(m_fp);
   fclose(m_fp);
   
   char new_log[256] = {0};
   char tail[16] = {0};
   ```

   先刷新和关闭当前的日志文件，然后构造新的日志文件名。如果是新的一天，直接拼接日期和文件名，如果是同一天但已达到最大行数，追加一个序号。

4. **构造新的日志文件路径：**

   ```c++
   snprintf_nowarn(tail,16,"%d_%02d_%02d_",my_tm.tm_year + 1900,my_tm.tm_mon+1,my_tm.tm_mday);
   if (m_today != my_tm.tm_mday) {
       snprintf_nowarn(new_log,255,"%s%s%s",dir_name,tail,log_name);
       m_today = my_tm.tm_mday;
       m_count = 0;
   } else {
       snprintf_nowarn(new_log,255,"%s%s%s.%lld",dir_name,tail,log_name,m_count / m_split_lines);
   }
   ```

   使用 `snprintf_nowarn` 构造新的日志文件路径，并更新 `m_today` 和 `m_count`。

5. **打开新的日志文件：**

   ```c++
   m_fp = fopen(new_log,"a");
   ```

   使用 `fopen` 打开新的日志文件，以追加写入的方式。

6. **构造日志级别前缀：**

   ```c++
   char s[16] = {0};
   switch(level){
       // ...
   }
   ```

   根据传入的日志级别（0: Debug, 1: Info, 2: Warn, 3: Error）选择对应的日志级别前缀。

7. **构造日志内容：**

   ```c++
   int n = snprintf(m_buf,48,"%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                     my_tm.tm_year+1900,my_tm.tm_mon+1,my_tm.tm_mday,
                           my_tm.tm_hour,my_tm.tm_min,my_tm.tm_sec,now.tv_usec,s);
   ```

   使用 `snprintf` 构造时间戳和日志级别前缀。

8. **构造日志正文：**

   ```c++
   va_list valst;
   va_start(valst,format);
   int m = vsnprintf(m_buf + n,m_log_buf_size -1, format, valst);
   va_end(valst);
   ```

   `va_list` 是一个指向参数的列表的类型，用于存储可变参数信息。`va_start` 宏用于初始化 `valst`，使其指向参数列表中的第一个可变参数。第一个参数是 `valst`，第二个参数是可变参数列表的最后一个已知的固定参数。`vsnprintf` 是一个可变参数版本的 `snprintf` 函数，用于将格式化的数据写入字符串。这里，它将可变参数根据指定的 `format` 格式化并写入 `m_buf` 字符数组中，起始位置是 `m_buf + n`，最大写入长度是 `m_log_buf_size - 1`。使用 `vsnprintf` 将可变参数格式化成字符串，追加到之前的时间戳后。

9. **添加换行符和异步/同步写入：**

   ```c++
   m_buf[n + m] = '\n';
   m_buf[n + m + 1] = '\0';
   
   if(m_is_async && !m_log_duque->full()){
       m_log_duque->push_back(m_buf);
   }else{
       fputs(m_buf,m_fp);
   }
   ```

   在字符串末尾添加换行符，并根据异步标志和队列是否满，选择是异步写入到阻塞队列还是同步写入到文件。最后，解锁临界区。

这段代码完成了日志的记录、切割和写入功能，支持同步和异步两种写入模式。

### 强制刷新文件缓冲 flush

```C++
// 强制刷新文件缓冲
void Log::flush(void){
    m_mutex.lock();  // 加锁，确保在多线程环境中对文件缓冲的操作是互斥的
    fflush(m_fp);    // 刷新文件缓冲，将缓冲区中的数据写入文件
    m_mutex.unlock();  // 解锁，释放互斥锁，允许其他线程对文件缓冲进行操作
}
```

