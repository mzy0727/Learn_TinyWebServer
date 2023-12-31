# 从零实现WebServer之定时器模块

## 客户端结构体

```c++
// 前向声明
class util_timer;

// 客户端数据结构体
struct client_data{
    sockaddr_in address;    // 客户端地址
    int sockfd;             // 客户socket
    util_timer *timer;      // 定时器
};
```

这段代码进行了对 `util_timer` 类的前向声明，并定义了一个客户端数据结构体 `client_data`。这样的设计是为了解决头文件的循环包含问题，即在头文件中需要引用对方的类或结构体，但又不能直接包含对方的头文件。

以下是代码的解释：

- `class util_timer;`：这是对 `util_timer` 类的前向声明。前向声明告诉编译器，在这个文件中会用到 `util_timer` 类，但具体的定义会在其他地方（可能是其他源文件）给出。
- `struct client_data`：这是一个结构体的定义，用于存储客户端的相关信息。结构体包含以下成员：
  - `sockaddr_in address;`：用于存储客户端的地址信息，包括 `IP` 地址和端口号。
  - `int sockfd;`：客户端的套接字文件描述符，用于标识连接。
  - `util_timer *timer;`：指向 `util_timer` 类型的指针，用于关联客户端的定时器。

通过前向声明，可以在该头文件中使用 `util_timer` 类型的指针而不需要包含完整的 `util_timer` 类定义。这有助于避免循环包含问题。

## 定时器类

这个定时器类通常用于实现定时任务，比如在网络编程中，可以使用定时器来管理连接的超时，定期清理不活跃的连接等。

```c++
// 定时器类
class util_timer{
public:
    util_timer():prev(nullptr), next(nullptr){}
public: 
    time_t expire;                  // 超时时间
    void (*cb_func)(client_data*);  // 回调函数
    client_data *user_data;         // 连接资源
    util_timer *prev;               // 前指针
    util_timer *next;               // 后指针
private:

};
```

这是一个定时器类 `util_timer` 的实现。以下是该类中的主要成员和说明：

- `util_timer()`：构造函数，用于初始化 `util_timer` 对象。在构造函数中，`prev` 和 `next` 被初始化为 `nullptr`，表示初始时还没有前一个和后一个定时器。
- `expire`：定时器的超时时间，使用 `time_t` 类型表示。当当前时间超过 `expire` 时，定时器即过期。
- `cb_func`：回调函数指针，指向一个以 `client_data*` 为参数的函数。当定时器到期时，将调用这个回调函数进行特定的操作。
- `user_data`：连接资源，通常是一个指向连接相关信息的结构体或对象的指针，用于在定时器到期时执行回调函数时提供必要的参数。
- `prev` 和 `next`：分别表示定时器链表中的前一个定时器和后一个定时器。这两个指针用于将定时器组织成一个双向链表，方便在定时器到期或者被删除时快速地进行链表操作。

## 定时器双向链表类

这个类主要用于管理定时器对象，通过添加、调整、删除、处理定时器来实现定时任务的功能。链表中的定时器按照过期时间升序排序，方便快速定位到期的定时器。

```c++
// 定时器双向链表，按过期时间升序排序
class sort_timer_lst{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);      // 添加定时器
    void adjust_timer(util_timer *timer);   // 调整定时器
    void del_timer(util_timer *timer);      // 删除定时器
    void tick();    // 定时任务处理函数

private:
    void add_timer(util_timer *timer, util_timer *lst_head);    // 辅助添加函数

    util_timer *head;   // 链表头
    util_timer *tail;   // 链表尾
};
```


这段代码定义了一个定时器双向链表类 `sort_timer_lst`，用于管理定时器对象，并按照过期时间升序排序。以下是代码的解释：

- `class sort_timer_lst`：定时器双向链表类的声明。
- `public` 成员函数：
  - `sort_timer_lst()`：构造函数，用于初始化链表头尾。
  - `~sort_timer_lst()`：析构函数，用于释放链表中的所有定时器对象。
  - `void add_timer(util_timer *timer)`：添加定时器到链表中。
  - `void adjust_timer(util_timer *timer)`：调整定时器，即将定时器从原位置移动到合适的位置。
  - `void del_timer(util_timer *timer)`：从链表中删除定时器。
  - `void tick()`：定时任务处理函数，用于处理到期的定时器。
- `private` 成员函数：
  - `void add_timer(util_timer *timer, util_timer *lst_head)`：辅助添加函数，用于在指定的链表头 `lst_head` 处添加定时器。
- 成员变量：
  - `util_timer *head;`：链表头指针，指向定时器链表的第一个节点。
  - `util_timer *tail;`：链表尾指针，指向定时器链表的最后一个节点。

### 构造函数 sort_timer_lst

```c++
// 定时器链表构造函数
sort_timer_lst::sort_timer_lst(){
    head = nullptr;
    tail = nullptr;
}
```

这是 `sort_timer_lst` 类的构造函数的实现，初始化链表的头尾指针：

构造函数 (`sort_timer_lst::sort_timer_lst()`)：

- 将链表头指针 `head` 和链表尾指针 `tail` 初始化为 `nullptr`，表示初始时链表为空。

### 析构函数 ~sort_timer_lst

```c++
// 定时器链表析构函数，析构所有定时器
sort_timer_lst::~sort_timer_lst(){
    util_timer *tmp = head;
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
    
}
```

这是 `sort_timer_lst` 类的析构函数的实现，用于释放链表中的所有定时器：

**析构函数 (`sort_timer_lst::~sort_timer_lst()`)：**

- 使用循环遍历整个链表，删除每个节点（定时器），释放相关内存。
- 遍历过程中，将链表头指针 `head` 指向下一个节点，然后删除当前节点。循环直到链表为空。

### 添加定时器（指定位置） add_timer

```c++
// 添加定时器，从给定head开始寻找可以插入的位置
void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head){
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    // 遍历链表，找到合适的位置插入
    while(tmp != nullptr){
        if(timer->expire < tmp->expire){
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    // 如果找到位置，则插入；否则插入到链表尾部
    if(tmp != nullptr){
        prev->next = timer;
        timer->prev = prev;
        timer->next = nullptr;
        tail = timer;
    }
}
```

- `add_timer` 函数用于向定时器链表中添加定时器。
- 参数 `timer` 是待添加的定时器，参数 `lst_head` 是链表的头节点，表示从哪个位置开始寻找插入位置。
- 首先，定义两个指针 `prev` 和 `tmp` 分别指向 `lst_head` 的下一个节点，初始化为链表头部。
- 然后，遍历链表，比较 `timer` 的超时时间和当前节点 `tmp` 的超时时间，找到合适的位置插入。
- 如果找到合适的位置，插入 `timer` 到 `prev` 和 `tmp` 之间，调整指针指向。
- 如果找不到合适的位置，将 `timer` 插入到链表尾部，更新 `tail` 指针。
- 这样，保证链表按照超时时间升序排列。

### 添加定时器（头结点） add_timer

```
// 添加定时器，按过期时间升序插入
void sort_timer_lst::add_timer(util_timer *timer){
    if(timer == nullptr)
        return ;
    if(head == nullptr){
        head = tail = timer;
        return ;
    }
    // 添加到链表头
    if(timer->expire < head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return ;
    }
    // 添加到链表中
    add_timer(timer,head);
}
```

这是 `sort_timer_lst` 类的添加定时器函数的实现，用于将定时器按照超时时间升序插入链表中：

**详解：**

1. **空链表情况：**
   - 如果链表为空（`head` 为 `nullptr`），直接将 `head` 和 `tail` 指向传入的定时器，表示链表中只有一个定时器。
2. **插入到链表头部情况：**
   - 如果传入的定时器的超时时间小于链表头部定时器的超时时间，将传入的定时器插入到链表头部，更新 `head` 指针。
3. **调用辅助函数 `add_timer`：**
   - 如果传入的定时器的超时时间大于等于链表头部定时器的超时时间，调用辅助函数 `add_timer` 进行插入操作。
4. **辅助函数 `add_timer`：**
   - 辅助函数的目标是在链表中找到合适的位置插入新的定时器，以保持链表的升序排序。
   - 通过比较定时器的超时时间，找到合适的位置插入新的定时器。
   - 如果找到合适的位置，插入定时器并调整前后指针，保持链表的有序性。
   - 如果找不到合适的位置，将定时器插入到链表尾部，更新 `tail` 指针。

### 调整定时器 adjust_timer

```c++
// 调整定时器
void sort_timer_lst::adjust_timer(util_timer *timer){
    if(timer == nullptr){
        return ;
    }
    util_timer *tmp = timer->next;
    if(tmp == nullptr || (timer->expire < tmp->expire)){
        return ;
    }
    // 先从链表中删除，再重新按顺序插入
    if(timer == head){
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer,head);
    }else{
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer,timer->next);
    }
}
```

这是 `sort_timer_lst` 类的调整定时器函数的实现，用于将链表中的定时器重新按照超时时间升序排列：

1. **空链表或只有一个定时器情况：**
   - 如果链表为空或只有一个定时器，无需调整，直接返回。
2. **比较当前定时器和下一个定时器的超时时间：**
   - 如果当前定时器的超时时间小于下一个定时器的超时时间，无需调整，直接返回。
3. **删除当前定时器并重新插入：**
   - 如果当前定时器是链表头部的定时器，从链表头部删除，然后调用添加定时器函数 `add_timer` 重新插入链表。
   - 如果当前定时器不是链表头部的定时器，从链表中删除，然后调用添加定时器函数 `add_timer` 从删除点的下一个位置开始重新插入链表。

### 删除定时器 del_timer

```c++
// 删除定时器
void sort_timer_lst::del_timer(util_timer *timer){
    if(timer == nullptr){
        return ;
    }
    if((timer == head) && (timer == tail)){
        delete timer;
        head = nullptr;
        tail = nullptr;
        return ;
    }
    if(timer == head){
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return ;
    }
    if(timer == tail){
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return ;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
```

这是 `sort_timer_lst` 类的删除定时器函数的实现，用于从链表中删除指定的定时器：

1. **空链表情况：**
   - 如果链表为空，直接返回，无需删除。
2. **删除链表头部定时器情况：**
   - 如果待删除的定时器是链表头部的定时器，将链表头部指针移动到下一个定时器，并删除原链表头部定时器。
3. **删除链表尾部定时器情况：**
   - 如果待删除的定时器是链表尾部的定时器，将链表尾部指针移动到上一个定时器，并删除原链表尾部定时器。
4. **一般情况：**
   - 如果待删除的定时器不是头部或尾部定时器，将待删除定时器的前后节点连接起来，然后删除待删除定时器。

### 定时任务处理函数 tick

```c++
// SIGALRM 信号每次被触发，主循环中调有一次定时任务处理函数，处理链表容器中到期的定时器
void sort_timer_lst::tick(){
    if(head == nullptr){
        return ;
    }
    time_t cur = time(nullptr);
    util_timer *tmp = head;
    while(tmp != nullptr){
        if(cur < tmp->expire){
            break;
        }
        // 过期时间小于当前时间，释放连接
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if(head != nullptr){
            head->prev = nullptr;
        }
        delete tmp;
        tmp = head;
    }
}

```

## 通用类

这个类主要包含了一些通用的工具函数，例如处理信号、初始化定时器、设置非阻塞模式、添加文件描述符到 `epoll` 事件表等。这些工具函数在服务器程序中可能会被频繁调用，用于处理与网络、定时器相关的操作。

```c++
// 通用类
class Utils{
public:
    Utils(){}
    ~Utils(){}

    // 静态函数避免this指针
    static void sig_handler(int sig);

    void init(int timeslot);
    int setnonblocking(int fd);
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);
    void addsig(int sig, void(*handler)(int), bool restart = true);
    void timer_handler();
    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;           // 本地套接字
    static int u_epollfd;           // epoll句柄
    sort_timer_lst m_timer_lst;     // 定时器链表
    int m_timeslot;                 // 定时时间
};
```

这段代码定义了一个通用类 `Utils`，主要包含了一些静态和非静态的工具函数。以下是对这个类的主要函数的解释：

- **构造函数和析构函数：**
  - `Utils()`：构造函数，用于初始化 `Utils` 类的实例。
  - `~Utils()`：析构函数，用于释放 `Utils` 类的实例。
- **静态成员变量：**
  - `static int *u_pipefd;`：本地套接字数组。
  - `static int u_epollfd;`：`epoll` 句柄。
- **非静态成员变量：**
  - `sort_timer_lst m_timer_lst;`：定时器链表对象。
  - `int m_timeslot;`：定时时间。
- **静态函数：**
  - `static void sig_handler(int sig)`：信号处理函数。用于处理指定信号 `sig` 的行为。
- **非静态函数：**
  - `void init(int timeslot)`：初始化函数，用于初始化定时时间。
  - `int setnonblocking(int fd)`：设置文件描述符 `fd` 为非阻塞模式。
  - `void addfd(int epollfd, int fd, bool one_shot, int TRIGMode)`：将文件描述符 `fd` 添加到 `epoll` 事件表中。
  - `void addsig(int sig, void(*handler)(int), bool restart = true)`：添加信号处理函数，指定信号 `sig` 的处理函数为 `handler`。
  - `void timer_handler()`：定时器处理函数，用于处理定时器到期的事件。
  - `void show_error(int connfd, const char *info)`：显示错误信息。

### 初始化函数 init

```c++
void Utils::init(int timeslot){
    m_timeslot = timeslot;
}
```

设置时间槽大小：

- 将函数参数 `timeslot` 赋值给类成员变量 `m_timeslot`，即设置定时器时间槽大小。

该函数用于初始化 `Utils` 类的定时器时间槽大小。

### 设置非阻塞 setnoblocking

```c++
// 设置非阻塞，读不到数据时返回-1，并设置errno为EAGAIN
int Utils::setnoblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
```

这是 `Utils` 类的设置非阻塞函数 `setnonblocking` 的实现，用于将指定文件描述符设置为非阻塞模式：

1. **获取旧的文件描述符状态：**
   - 使用 `fcntl` 函数的 `F_GETFL` 命令获取文件描述符 `fd` 的当前状态，并将其保存在变量 `old_option` 中。
2. **设置新的文件描述符状态：**
   - 使用位运算将 `O_NONBLOCK` 标志加入到 `old_option` 中，形成新的文件描述符状态 `new_option`。
3. **将新的文件描述符状态应用到文件描述符：**
   - 使用 `fcntl` 函数的 `F_SETFL` 命令，将新的文件描述符状态 `new_option` 应用到文件描述符 `fd` 上。
4. **返回旧的文件描述符状态：**
   - 返回调用该函数之前文件描述符的状态，即返回 `old_option`。

### 向epoll注册事件 addfd

```c++
// 向epoll注册事件
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode){
    epoll_event event;
    event.data.fd = fd;
    // ET模式
    if(TRIGMode == 1){
        // EPOLLRDHUP 对端关闭
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }else{  // 默认LT模式
        event.events = EPOLLIN | EPOLLRDHUP;
    }

    // 只触发一次
    if(one_shot){
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    // 设置非阻塞
    setnoblocking(fd);
}
```

这是 `Utils` 类的向 `epoll` 注册事件的函数 `addfd` 的实现，用于向指定的`epoll`实例注册事件：

**参数：**

- `epollfd`:`epoll` 实例的文件描述符。
- `fd`: 要注册的文件描述符。
- `one_shot`: 是否设置为一次性事件。
- `TRIGMode`: 触发模式，1 表示使用 ET 模式，0 表示使用默认的 `LT` 模式。

**详解：**

1. **初始化 `epoll_event` 结构体：**
   - 创建一个 `epoll_event` 结构体实例 `event`，并将要注册的文件描述符 `fd` 存储在 `event.data.fd` 中。
2. **设置事件类型：**
   - 根据触发模式 `TRIGMode` 的值设置事件类型，如果为 1（ET 模式），则设置 `EPOLLIN`、`EPOLLET` 和 `EPOLLRDHUP` 事件；否则，默认使用 LT 模式，只设置 `EPOLLIN` 和 `EPOLLRDHUP` 事件。
3. **设置一次性事件：**
   - 如果 `one_shot` 为 `true`，则将 `EPOLLONESHOT` 标志添加到事件类型中，表示这是一个一次性事件。
4. **向 epoll 实例注册事件：**
   - 使用 `epoll_ctl` 函数向 `epoll`实例注册事件，将事件添加到事件列表中。
5. **设置文件描述符为非阻塞模式：**
   - 调用 `setnonblocking` 函数将文件描述符 `fd` 设置为非阻塞模式。

### 信号处理函数 sig_handler

```c++
// 信号处理函数
void Utils::sig_handler(int sig){
    // 为保证函数的可重入性，保留原来的errno
    // 可重入性表示中断后再次进入该函数，环境变量与之前相同，不会丢失数据
    int save_errno = errno;
    int msg = sig;

    // 将信号从管道写端写入，以字符类型传输
    // 回到主循环处理信号业务逻辑
    send(u_epollfd[1],(char *)&msg,1,0);

    // 恢复errno
    errno = save_errno;
}
```

这是 `Utils` 类中的信号处理函数 `sig_handler` 的实现，用于处理捕获到的信号：

**参数：**

- `sig`: 捕获到的信号。

**详解：**

1. **保留原始 `errno`：**
   - 为了保证函数的可重入性，首先保存当前的 `errno`，以便后续恢复。
2. **将信号写入管道：**
   - 将捕获到的信号 `sig` 通过管道的写端（`u_epollfd[1]`）写入，以字符类型传输。
3. **恢复 `errno`：**
   - 恢复之前保存的 `errno`，保证函数的可重入性。

该函数将捕获到的信号发送到主循环中，通过管道实现了信号的跨线程通信，主循环将会从管道的读端读取到信号，并进行相应的处理逻辑。这种机制常用于处理信号与主程序逻辑的异步通信。

### 设置信号 addsig

```c++
// 设置信号
void Utils::addsig(int sig, void (*handler)(int), bool restart){
    // 创建sigaction结构体
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));

    // 设置信号处理函数
    sa.sa_handler = handler;
    if(restart){
        // 使被信号打断的系统调用重新自动发起
        sa.sa_flags |= SA_RESTART;
    }
    // 对信号集初始化，将所有信号添加到信号集中
    sigfillset(&sa.sa_mask);

    // 设置信号， -1 表示有错误发生
    assert(sigaction(sig,&sa,NULL) != -1);
}
```

这是 `Utils` 类中的设置信号函数 `addsig` 的实现，用于设置捕获到的信号的处理函数：

**参数：**

- `sig`: 要设置的信号。
- `handler`: 信号处理函数的指针。
- `restart`: 是否开启对系统调用的自动重启。

**详解：**

1. **初始化 `sigaction` 结构体：**
   - 创建 `sigaction` 结构体并用 `memset` 初始化。
2. **设置信号处理函数：**
   - 将信号处理函数指针设置为传入的 `handler`。
3. **设置自动重启标志（可选）：**
   - 如果 `restart` 为 `true`，则设置 `SA_RESTART` 标志，使被信号打断的系统调用重新自动发起。
4. **初始化信号集：**
   - 使用 `sigfillset` 将所有信号添加到信号集中，以确保在信号处理函数执行期间，所有信号都被阻塞。
5. **设置信号处理：**
   - 使用 `sigaction` 函数将指定信号与设置好的 `sigaction` 结构体关联起来，实现信号的处理函数设置。

### 定时器触发 timer_handler

```c++
// 定时器触发
void Utils::timer_handler(){
    // 处理连接链表
    m_timer_lst.tick();
    // 重新设置时钟
    alarm(m_timeslot);
}
```

这是 `Utils` 类中的定时器处理函数 `timer_handler` 的实现，用于处理定时器链表中的定时器触发事件，并重新设置时钟：

1. **处理连接链表：**
   - 调用 `m_timer_lst.tick()` 处理定时器链表中的定时器触发事件。该函数的具体实现逻辑是遍历定时器链表，找到超时的定时器并执行相应的操作。
2. **重新设置时钟：**
   - 调用 `alarm(m_timeslot)` 重新设置时钟，以确保下一次定时器的触发事件。

该函数的执行流程是在定时器触发时被调用，首先处理定时器链表中的定时器触发事件，然后重新设置时钟，为下一次触发事件做准备。这样可以保证定时器链表中的定时器得到及时处理。

### 错误信息显示函数 show_error

```c++
void Utils::show_error(int connfd, const char *info){
    send(connfd,info,strlen(info),0);
    close(connfd);
}
```

这是 `Utils` 类中的错误信息显示函数 `show_error` 的实现，用于向指定连接发送错误信息并关闭连接：

**详解：**

1. **发送错误信息：**
   - 使用 `send` 函数向连接（`connfd`）发送错误信息（`info`）。`strlen(info)` 表示发送 `info` 字符串的长度。
2. **关闭连接：**
   - 使用 `close` 函数关闭连接，确保错误信息发送后连接得到关闭。

该函数在处理连接过程中出现错误时使用，通过发送错误信息告知连接一些错误详情，并关闭连接以终止与客户端的通信。

### 回调函数 cb_func

```c++
int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

// 定时器回调函数，释放客户端连接
void cb_func(client_data *user_data){
    // 删除非活动连接在socket上的注册事件
    epoll_ctl(Utils::u_epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
    assert(user_data);

    // 关闭socket
    close(user_data->sockfd);

    // 减少连接数
    
}
```

这是 `Utils` 类中的回调函数 `cb_func` 的实现，用于释放客户端连接资源：

**详解：**

1. **删除 `epoll` 注册事件：**
   - 使用 `epoll_ctl` 函数删除非活动连接在 epoll 上的注册事件。这样，该连接将不再被 epoll 监听。
2. **关闭连接 socket：**
   - 使用 `close` 函数关闭客户端连接的 socket。
3. **减少连接数：**
   - 减少连接数，这可能是通过某种计数机制来实现的，具体实现可能在其他部分的代码中。

该回调函数通常在定时器处理过程中被调用，用于释放非活动连接的资源。

