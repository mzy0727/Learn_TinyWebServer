# 从零实现WebServer之Server类

{% note success %}

- `log_write`: 写入日志。
- `sql_pool`: 管理数据库连接池。
- `thread_pool`: 管理线程池。
- `eventListen`: 监听监听套接字上的事件。
- `eventLoop`: 进入主事件循环。
- `timer`: 为客户连接设置计时器。
- `adjust_timer`: 调整计时器的过期时间。
- `deal_timer`: 处理特定套接字的计时器过期。
- `dealclinetdata`: 处理客户端数据。
- `dealwithsignal`: 处理信号，处理超时和停止服务器。
- `dealwithread`: 处理读事件。
- `dealwithwrite`: 处理写事件。

{% endnote %}

## Sever类

### 流程图

![整体框架](https://glf-1309623969.cos.ap-shanghai.myqcloud.com/img/arch.jpg)

### 构造函数 WebServer

```c++
WebServer::WebServer(string user, string password, string database_name, const char *root,
                     int port, int close_log, int async_log, int sql_num,
                     int thread_num, int actor_model, int trig_mode, int opt_linger) {
    // 初始化成员变量
    m_user = user;
    m_passWord = password;
    m_databaseName = database_name;
    m_root = root;
    m_port = port;
    m_close_log = close_log;
    m_async_log = async_log;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_actormodel = actor_model;

    // 设置监听触发模式和连接触发模式
    m_listen_trig_mode = trig_mode & 2;
    m_conn_trig_mode = trig_mode & 1;

    m_opt_linger = opt_linger;

    // 分配空间并初始化http_conn和client_data数组
    users = new http_conn[MAX_FD];
    users_timer = new client_data[MAX_FD];

    // 写入日志、初始化数据库连接池、初始化线程池
    log_write();
    sql_pool();
    thread_pool();
}
```

- 通过参数初始化成员变量。
- 根据传入的 `trig_mode` 参数设置监听触发模式和连接触发模式。
- 分配空间并初始化 `http_conn` 和 `client_data` 数组。
- 调用 `log_write`、`sql_pool` 和 `thread_pool` 函数来进行日志写入、初始化数据库连接池和初始化线程池。

在整个服务器初始化过程中，这个构造函数负责一些基本的配置、资源的分配和初始化。

### 析构函数 ~WebServer

```c++
WebServer::~WebServer() {
    // 关闭 epoll 句柄、监听套接字和信号管道
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);

    // 释放 http_conn 和 client_data 数组的内存
    delete[] users;
    delete[] users_timer;

    // 释放线程池和数据库连接池的内存
    delete m_pool;
    delete m_connPool;
}
```

- 关闭 epoll 句柄、监听套接字和信号管道。
- 释放 `http_conn` 和 `client_data` 数组的内存。
- 释放线程池和数据库连接池的内存。

这个析构函数用于在对象生命周期结束时执行清理和释放资源的操作。

### 初始化日志 log_write

```c++
// 初始化日志
void WebServer::log_write() {
    if (m_close_log == 0) {
        // 如果不关闭日志，则进行日志初始化
        if (m_async_log == 1) {
            // 异步写入，需要设置阻塞队列长度
            Log::get_instance()->init("./log/ServerLog", 2000, 800000, 800);
        } else {
            // 同步写入
            Log::get_instance()->init("./log/ServerLog", 2000, 800000, 0);
        }
    }
}
```

- 首先检查是否关闭日志（`m_close_log == 0`）。
- 如果不关闭日志，根据是否异步日志`m_async_log == 1`，选择相应的日志初始化方式。
  - 对于异步写入，需要设置阻塞队列的长度。
  - 对于同步写入，不需要阻塞队列。

这个函数用于初始化日志模块，根据配置选择同步或异步写入，并设置相应的参数。这样可以在需要时记录服务器的运行日志。

### 初始化数据库连接池 sql_pool

```c++
// 初始化数据库连接池
void WebServer::sql_pool() {
    // 获取数据库连接池的单例
    m_connPool = connection_pool::get_instance();

    // 初始化数据库连接池
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log);

    // 通过数据库连接池初始化用户表
    users->initmysql_result(m_connPool);
}
```

- 获取数据库连接池的单例对象。
- 使用传入的配置信息初始化数据库连接池，包括数据库地址、用户名、密码、数据库名、端口号、连接池大小等参数。
- 调用 `initmysql_result` 函数初始化用户表，该函数可能会使用数据库连接池执行相应的查询操作。

这个函数用于初始化数据库连接池，并通过连接池初始化用户表，以便服务器在运行过程中能够使用数据库连接进行数据操作。

### 初始化线程池 thread_pool

```c++
// 初始化线程池
void WebServer::thread_pool() {
    // 创建一个线程池对象，用于处理 http_conn 类型的任务
    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}
```

- 创建一个新的线程池对象，该线程池针对 `http_conn` 类型的任务进行处理。
- 使用传入的参数 `m_actormodel`、`m_connPool` 和 `m_thread_num` 初始化线程池。

总体来说，这个函数用于初始化服务器的线程池，为处理 HTTP 连接请求提供并发处理的能力。线程池是一种常用的技术，可以有效地管理和复用线程，提高服务器的性能。

### 监听事件 eventListen

```c++
// 监听事件，网络编程基础步骤
void WebServer::eventListen() {
    // 创建一个 IPv4 面向连接的套接字
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    // 设置优雅关闭连接
    if (m_opt_linger) {
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    // 允许端口重用
    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    // 设置监听地址和端口
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    // 绑定地址和端口
    int ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    // 监听连接请求
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    // 创建 epoll 句柄
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    // 初始化定时器
    utils.init(TIMESLOT);

    // 注册监听套接字事件并设置为非阻塞
    utils.addfd(m_epollfd, m_listenfd, false, m_listen_trig_mode);
    http_conn::m_epollfd = m_epollfd;

    // 创建一对套接字，用于统一事件源
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    // 忽略SIGPIPE信号
    utils.addsig(SIGPIPE, SIG_IGN);
    // 处理时钟信号
    utils.addsig(SIGALRM, utils.sig_handler, false);
    // 处理终止信号
    utils.addsig(SIGTERM, utils.sig_handler, false);
    // 处理终止信号
    utils.addsig(SIGABRT, utils.sig_handler, false);

    // 设置定时器
    alarm(TIMESLOT);

    // 将一些静态成员变量设置为全局变量，以在信号处理函数中使用
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}
```

1. **创建监听套接字：**

   ```c++
   m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
   assert(m_listenfd >= 0);
   ```

   - 使用IPv4协议（`PF_INET`）创建一个面向连接的套接字（`SOCK_STREAM`）。
   - 断言确保套接字创建成功。

2. **设置优雅关闭连接：**

   ```c++
   if (m_opt_linger) {
       struct linger tmp = {1, 1};
       setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
   }
   ```

   - 如果启用了优雅关闭选项（`m_opt_linger`为真），则设置 `SO_LINGER` 选项，以确保在关闭连接时等待数据发送完毕。

3. **端口重用：**

   ```c++
   int flag = 1;
   setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
   ```

   - 设置 `SO_REUSEADDR` 选项，允许重用端口。

4. **绑定地址和端口：**

   ```c++
   struct sockaddr_in address;
   bzero(&address, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = htonl(INADDR_ANY);
   address.sin_port = htons(m_port);
   
   int ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
   assert(ret >= 0);
   ```

   - 设置服务器地址结构体，并将套接字与指定的地址和端口进行绑定。
   - 断言确保绑定成功。

5. **监听连接请求：**

   ```c++
   ret = listen(m_listenfd, 5);
   assert(ret >= 0);
   ```

   - 开始监听连接请求，设置监听队列的最大长度为5。
   - 断言确保监听成功。

6. **创建 epoll 句柄：**

   ```c++
   m_epollfd = epoll_create(5);
   assert(m_epollfd != -1);
   ```

   - 创建 epoll 句柄，用于事件管理。
   - 断言确保创建成功。

7. **初始化定时器：**

   ```c++
   utils.init(TIMESLOT);
   ```

   - 使用 `utils` 对象初始化定时器，设置最小超时单位为 `TIMESLOT`。

8. **注册监听套接字事件：**

   ```c++
   utils.addfd(m_epollfd, m_listenfd, false, m_listen_trig_mode);
   http_conn::m_epollfd = m_epollfd;
   ```

   - 使用 `utils` 对象注册监听套接字事件，并设置为非阻塞模式。
   - 将 `http_conn` 类的静态成员变量 `m_epollfd` 设置为当前 epoll 句柄。

9. **创建统一事件源：**

   ```c++
   ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
   assert(ret != -1);
   utils.setnonblocking(m_pipefd[1]);
   utils.addfd(m_epollfd, m_pipefd[0], false, 0);
   ```

   - 创建一对套接字，用于统一事件源，其中一个套接字用于写，另一个用于读。
   - 将写入端设置为非阻塞，并将其加入 epoll 监听。

10. **处理信号：**

```c++
utils.addsig(SIGPIPE, SIG_IGN);
utils.addsig(SIGALRM, utils.sig_handler, false);
utils.addsig(SIGTERM, utils.sig_handler, false);
utils.addsig(SIGABRT, utils.sig_handler, false);
```

- 忽略 `SIGPIPE` 信号，避免因向已关闭的连接发送数据而导致进程退出。
- 处理时钟信号、终止信号、终止信号，分别执行指定的信号处理函数。

1. **设置定时器：**

```c++
alarm(TIMESLOT);
```

- 设置 `TIMESLOT` 秒后发送 `SIGALRM` 信号，触发定时器。

1. **设置全局变量：**

```c++
Utils::u_pipefd = m_pipefd;
Utils::u_epollfd = m_epollfd;
```

- 将一些静态成员变量设置为全局变量，以在信号处理函数中使用。

这个函数完成了服务器的基本初始化，包括创建套接字、绑定地址和端口、开始监听、创建 epoll 句柄等。同时，它还设置了定时器、信号处理函数等，为后续的事件循环做好准备。

{% note success %}

优雅关闭连接是指在服务器或客户端主动关闭连接时，通过一些合理的方式来确保数据的完整性和可靠性。优雅关闭连接的目标是在关闭连接的同时尽量保持数据的完整性，防止数据的丢失或被截断。

一般情况下，TCP连接的关闭过程是通过TCP的四次握手来完成的。在这个过程中，有两个方向上的连接，一个是客户端到服务器端的连接（客户端发送FIN，服务器端回应ACK），另一个是服务器端到客户端的连接（服务器端发送FIN，客户端回应ACK）。通过这个四次握手，连接可以被优雅地关闭。

{% endnote %}

{% note success %}

统一事件源"是一种多线程或多进程编程模型中的设计模式，用于将不同的事件（通常是信号）集中到一个共享的事件源中，以便于集中处理。这样的设计模式主要用于在多线程或多进程环境中处理异步事件，避免多个线程或进程分别处理相同的事件。

在上下文中，特别是在网络编程中，"统一事件源"通常用于以下目的：

1. **信号处理：** 在UNIX-like系统中，信号是一种异步的通知机制，用于通知进程发生了某些事件，如中断、错误等。通过统一事件源，可以将多个信号通过一个特殊的套接字进行传递，然后通过事件循环集中处理。
2. **线程间通信：** 在多线程编程中，不同的线程可能需要相互通信，例如，一个线程产生了某个事件，而另一个线程需要处理这个事件。通过一个共享的事件源，线程可以将事件放置在事件队列中，其他线程则可以从队列中取出事件进行处理。

在上述 `WebServer::eventListen` 函数中，通过创建一对套接字（`m_pipefd`），其中一个用于写入，另一个用于读取，实现了统一事件源的机制。这对套接字被加入到 epoll 监听中，用于通知主循环中的线程或进程发生了某些事件，以进行相应的处理。这种机制是为了避免在多线程或多进程环境中产生竞争条件，使得事件处理更加集中和可控。

{% endnote %}



```c++
// 设置客户和定时器
void WebServer::timer(int connfd, struct sockaddr_in client_address) {
    // 初始化客户
    users[connfd].init(connfd, client_address, m_root, m_conn_trig_mode, m_close_log, m_user, m_passWord, m_databaseName);

    // 创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;

    // 创建新的定时器对象
    util_timer *timer = new util_timer;
    
    // 设置定时器的用户数据和回调函数
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;

    // 计算超时时间，设置为当前时间加上3倍的最小超时单位
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;

    // 将定时器与用户数据绑定
    users_timer[connfd].timer = timer;

    // 将定时器添加到定时器链表中
    utils.m_timer_lst.add_timer(timer);
}
```

1. **初始化客户：**

   ```c++
   users[connfd].init(connfd, client_address, m_root, m_conn_trig_mode, m_close_log, m_user, m_passWord, m_databaseName);
   ```

   - 使用 `connfd` 和 `client_address` 等参数初始化 `http_conn` 类对象，表示一个客户端连接。

2. **创建定时器：**

   ```c++
   util_timer *timer = new util_timer;
   ```

   - 创建一个新的 `util_timer` 对象，用于表示定时器。

3. **设置定时器的用户数据和回调函数：**

   ```c++
   timer->user_data = &users_timer[connfd];
   timer->cb_func = cb_func;
   ```

   - 将定时器的用户数据设置为当前客户端对应的 `users_timer` 对象，表示定时器与客户端连接关联。
   - 设置定时器的回调函数为 `cb_func`，表示定时器超时时要执行的回调函数。

4. **设置定时器超时时间：**

   ```c++
   time_t cur = time(NULL);
   timer->expire = cur + 3 * TIMESLOT;
   ```

   - 计算定时器的超时时间，设置为当前时间加上3倍的最小超时单位。

5. **将定时器与用户数据绑定：**

   ```c++
   users_timer[connfd].timer = timer;
   ```

   - 将定时器与当前客户端连接关联，以便后续能够通过客户端连接找到相应的定时器。

6. **将定时器添加到定时器链表中：**

   ```c++
   utils.m_timer_lst.add_timer(timer);
   ```

   - 将新创建的定时器添加到定时器链表中，以便后续对定时器的管理和触发。

这个函数的目的是为了初始化客户端连接以及与之关联的定时器，确保在一定时间内没有数据交互时能够及时关闭连接。这种机制通常用于处理长连接中的超时问题。

### 调整定时器 adjust_timer

```c++
//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void WebServer::adjust_timer(util_timer *timer){
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}
```

1. `time_t cur = time(NULL);`: 获取当前时间，并存储在变量 `cur` 中。
2. `timer->expire = cur + 3 * TIMESLOT;`: 将定时器的过期时间设置为当前时间加上3个时间单位（`TIMESLOT` 的值）。这样，定时器被往后延迟了3个单位。
3. `utils.m_timer_lst.adjust_timer(timer);`: 调用 `utils` 对象中的 `m_timer_lst` 成员的 `adjust_timer` 函数，该函数用于对定时器在链表上的位置进行调整。这可能涉及到将定时器从一个位置移动到另一个位置，以确保定时器链表按照过期时间的顺序排列。
4. `LOG_INFO("%s", "adjust timer once");`: 记录一条日志，表示定时器已被调整一次。

这段代码的目的是在数据传输时延迟定时器的触发时间，并且对定时器在链表上的位置进行调整，以确保定时器链表的有序性。这种有序性通常用于有效地处理定时器事件，例如处理连接超时等情况。

### 删除计时器 deal_timer

```c++
// 释放连接，删除计时器
void WebServer::deal_timer(util_timer *timer, int sockfd){
    timer->cb_func(&users_timer[sockfd]);
    if(timer){
        utils.m_timer_lst.del_timer(timer);
    }

    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}
```

1. `timer->cb_func(&users_timer[sockfd]);`: 调用定时器 `timer` 中的回调函数 `cb_func`，并传递 `users_timer[sockfd]` 作为参数。这里假设回调函数的目的是处理与连接相关的操作，可能是释放资源或执行其他必要的清理工作。
2. `if(timer) { utils.m_timer_lst.del_timer(timer); }`: 首先检查定时器是否存在（非空）。如果定时器存在，就调用 `utils` 对象中的 `m_timer_lst` 成员的 `del_timer` 函数，该函数用于从定时器链表中删除指定的定时器。这样，已经处理过的定时器就被移除了，防止在未来的定时器触发时再次执行相同的处理逻辑。
3. `LOG_INFO("close fd %d", users_timer[sockfd].sockfd);`: 记录一条日志，表示关闭了文件描述符（socket）为 `users_timer[sockfd].sockfd` 的连接。

这段代码用于在定时器触发时处理与连接相关的操作，包括调用回调函数、从定时器链表中删除定时器，并记录相关的日志信息。这是一个常见的定时器处理模式，用于清理连接资源等操作。

### 接受新客户连接 dealclinetdata

当处理新的客户端连接时，`WebServer::dealclinetdata` 函数的作用主要是接受客户端连接，执行一些处理，并根据不同的触发模式（`m_listen_trig_mode`）进行逻辑判断。

```c++
// 接受新客户连接
bool WebServer::dealclinetdata(){
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if(m_listen_trig_mode == 0){
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if(connfd < 0){
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if(http_conn::m_user_count >= MAX_FD){
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, client_address);
    }
    else{
        while(1){
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if(connfd < 0){
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if(http_conn::m_user_count >= MAX_FD){
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}
```

1. `struct sockaddr_in client_address;`: 创建一个结构体 `client_address`，用于存储客户端的地址信息。
2. `socklen_t client_addrlength = sizeof(client_address);`: 获取客户端地址结构体的大小。
3. `if(m_listen_trig_mode == 0) { ... }`: 如果触发模式为 0，表示非阻塞模式（可能是边缘触发），则执行以下逻辑：
   - `int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);`: 调用 `accept` 函数接受新的客户端连接，将客户端地址信息存储在 `client_address` 中，连接的文件描述符存储在 `connfd` 中。
   - 如果连接失败（`connfd < 0`），记录错误信息并返回 `false`。
   - 如果当前连接数已经达到最大值（`MAX_FD`），通过向客户端返回服务器繁忙的错误信息，记录错误信息并返回 `false`。
   - 调用 `timer` 函数，处理与连接相关的定时器逻辑。
4. `else { ... }`: 如果触发模式不为 0，表示可能是水平触发，执行以下逻辑：
   - 在一个无限循环中，不断尝试接受新的客户端连接。
   - 对每个连接执行类似上述的逻辑：如果连接失败或者当前连接数已经达到最大值，跳出循环。
5. `return true;`: 如果执行了一次连接处理或者在循环中成功处理了连接，返回 `true`。

这个函数是用于处理新客户端连接的，具体操作包括接受连接、处理错误、处理服务器繁忙情况、执行定时器逻辑等。函数的返回值表示是否成功处理了连接。在非阻塞模式下，可能只处理一个连接；在水平触发模式下，可能会循环处理多个连接，直到没有新连接或者达到最大连接数。

### 处理信号 dealwithsignal

```c++
// 处理信号
bool WebServer::dealwithsignal(bool &timeout, bool &stop_server){
    int ret = 0;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if(ret == -1){
        return false;
    }
    else if(ret == 0){
        return false;
    }
    else{
        for(int i = 0; i < ret; ++i){
            switch(signals[i]){
                case SIGALRM: timeout = true; break;
                case SIGTERM: stop_server = true; break;
                case SIGABRT: stop_server = true; break;
            }
        }
    }
    return true;
}
```

1. `int ret = recv(m_pipefd[0], signals, sizeof(signals), 0);`: 通过管道 `m_pipefd` 接收信号。这个管道可能是通过调用 `pipe` 函数创建的，用于在信号处理函数中传递信号信息。
2. 如果 `ret == -1`，表示接收出错，函数返回 `false`。
3. 如果 `ret == 0`，表示没有信号数据可读，函数返回 `false`。
4. 如果有信号数据可读，则遍历接收到的信号数据，根据信号类型执行相应的操作：
   - 如果是 `SIGALRM` 信号，将 `timeout` 标志设置为 `true`。
   - 如果是 `SIGTERM` 或 `SIGABRT` 信号，将 `stop_server` 标志设置为 `true`。
5. 返回 `true` 表示信号处理成功。

这段代码通常用于多线程或异步信号处理的情况，通过在主线程或主循环中接收信号，并根据接收到的信号类型执行相应的操作。在这里，通过管道实现了信号的传递，通过设置标志位来通知主程序发生了相应的信号事件，比如超时（`SIGALRM`）或终止服务器（`SIGTERM` 或 `SIGABRT`）。

### 处理读 dealwithread

这段代码用于处理读事件。根据服务器的模型（`reactor `模型或 `proactor `模型），执行不同的处理逻辑。

```c++
// 处理读
void WebServer::dealwithread(int sockfd){
    util_timer *timer = users_timer[sockfd].timer;

    // reactor
    if(m_actormodel == 1){
        // 更新计时器
        if(timer){
            adjust_timer(timer);
        }

        // 若监测到读事件，将该事件放入请求队列
        m_pool->append(users + sockfd, 0);

        // 等待工作线程读
        while(true){
            if(users[sockfd].improv == 1){
                // 读失败
                if(users[sockfd].timer_flag == 1){
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    // proactor
    else{
        if(users[sockfd].read_once()){
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            // 更新计时器
            if(timer){
                adjust_timer(timer);
            }

            // 若监测到读事件，将该事件放入请求队列
            m_pool->append_p(users + sockfd);  
        }
        // 读失败
        else{
            deal_timer(timer, sockfd);
        }
    }
}
```

1. `util_timer *timer = users_timer[sockfd].timer;`: 获取与当前连接关联的定时器。
2. `if(m_actormodel == 1) { ... }`: 如果服务器采用` reactor `模型：
   - `adjust_timer(timer);`: 调用 `adjust_timer` 函数，更新定时器的过期时间。
   - `m_pool->append(users + sockfd, 0);`: 将读事件放入请求队列。这里可能是一个任务队列，用于在工作线程中处理读事件。
   - 使用循环等待工作线程的处理结果。如果读事件处理失败（`users[sockfd].improv == 1`），则可能触发定时器处理。
3. `else { ... }`: 如果服务器采用` proactor `模型：
   - `if(users[sockfd].read_once()) { ... }`: 如果成功读取数据：
     - `adjust_timer(timer);`: 调用 `adjust_timer` 函数，更新定时器的过期时间。
     - `m_pool->append_p(users + sockfd);`: 将读事件放入请求队列。这里可能是一个任务队列，用于在工作线程中处理读事件。
   - 如果读事件失败，可能是连接出错，调用 `deal_timer(timer, sockfd);` 处理定时器。

这段代码根据服务器模型的不同，采用不同的处理方式。在` reactor `模型下，通过事件队列和工作线程来异步处理读事件，而在` proactor` 模型下，直接在主线程中处理读事件。

### 处理写 dealwithwrite

当处理写事件时，`WebServer::dealwithwrite` 函数的目标是根据服务器的模型（reactor 模型或 proactor 模型）执行不同的处理逻辑。

```c++
// 处理写
void WebServer::dealwithwrite(int sockfd){
    util_timer *timer = users_timer[sockfd].timer;
    
    // reactor
    if(m_actormodel == 1){
        if(timer){
            adjust_timer(timer);
        }

        m_pool->append(users + sockfd, 1);

        while(true){
            if(users[sockfd].improv == 1){
                if(users[sockfd].timer_flag == 1){
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else{
        // proactor
        if(users[sockfd].write()){
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            if(timer){
                adjust_timer(timer);
            }
        }
        else{
            deal_timer(timer, sockfd);
        }
    }
}
```

1. `util_timer *timer = users_timer[sockfd].timer;`: 获取当前连接关联的定时器，该定时器可能用于处理超时事件。

2. **Reactor 模型**：

   ```
   cppCopy codeif(m_actormodel == 1){
       if(timer){
           adjust_timer(timer);
       }
       m_pool->append(users + sockfd, 1);
       while(true){
           if(users[sockfd].improv == 1){
               if(users[sockfd].timer_flag == 1){
                   deal_timer(timer, sockfd);
                   users[sockfd].timer_flag = 0;
               }
               users[sockfd].improv = 0;
               break;
           }
       }
   }
   ```

   - 如果采用` reactor` 模型，首先调用 `adjust_timer` 函数，更新定时器的过期时间。
   - 将写事件加入请求队列，可能是一个任务队列，用于在工作线程中异步处理写事件。
   - 通过循环等待工作线程的处理结果。如果 `users[sockfd].improv` 为 1，表示写事件处理失败。如果 `users[sockfd].timer_flag` 也为 1，表示定时器需要处理，就调用 `deal_timer` 函数来处理定时器。然后重置相关标志位，跳出循环。

3. **Proactor 模型**：

   ```
   cppCopy codeelse{
       if(users[sockfd].write()){
           LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
           if(timer){
               adjust_timer(timer);
           }
       }
       else{
           deal_timer(timer, sockfd);
       }
   }
   ```

   - 如果采用` proactor` 模型，通过 `write `函数尝试发送数据。如果发送成功：
     - 记录日志，表示成功发送数据。
     - 调用 `adjust_timer` 函数，更新定时器的过期时间。
   - 如果发送失败，可能是连接出错，就调用 `deal_timer` 函数来处理定时器。

这段代码负责处理写事件，具体的处理方式根据服务器模型的不同而异。在 reactor 模型下，通过异步任务队列和工作线程来处理，而在 proactor 模型下，直接在主线程中处理。

### 循环处理事件 eventLoop

```c++
// 循环处理事件
void WebServer::eventLoop(){
    bool timeout = false;
    bool stop_server = false;

    while(!stop_server){
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        // epoll_wait会被信号打断，返回-1并设置errno=EINTR
        if(number < 0 && errno != EINTR){
            LOG_ERROR("%s", "epoll failure");
            break;
        }
        // 遍历事件
        for(int i = 0; i < number; i++){
            int sockfd = events[i].data.fd;
            // 处理新到的客户连接
            if(sockfd == m_listenfd){
                bool flag = dealclinetdata();
                if(flag == false)
                    continue;
            }
            // 对端关闭
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                // 服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            // 处理信号
            else if((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN)){
                bool flag = dealwithsignal(timeout, stop_server);
                if(flag == false)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            // 处理客户连接上接收到的数据
            else if(events[i].events & EPOLLIN){
                dealwithread(sockfd);
            }
            // 处理客户连接上要发送的数据
            else if(events[i].events & EPOLLOUT){
                dealwithwrite(sockfd);
            }
        }
        // 时钟到时
        if(timeout){
            utils.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}
```

1. `bool timeout = false;` 和 `bool stop_server = false;`: 两个标志位，`timeout` 表示是否发生超时，`stop_server` 表示是否要停止服务器。
2. `while(!stop_server) { ... }`: 在一个循环中处理事件，直到 `stop_server` 标志被设置为 `true`。
3. `int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);`: 使用 `epoll_wait` 函数等待事件的发生。`m_epollfd` 是 epoll 文件描述符，`events` 用于存储发生的事件，`MAX_EVENT_NUMBER` 表示最大事件数量，-1 表示无限等待。函数返回发生的事件数量。
4. `if(number < 0 && errno != EINTR) { ... }`: 如果 `epoll_wait` 函数返回负值并且错误码不是 `EINTR`，表示发生了错误，记录错误信息并跳出循环。
5. `for(int i = 0; i < number; i++) { ... }`: 遍历发生的事件。
   - `int sockfd = events[i].data.fd;`: 获取与事件关联的文件描述符。
   - `if(sockfd == m_listenfd) { ... }`: 如果是监听 socket 产生的事件，表示有新的客户连接请求，调用 `dealclinetdata` 处理。
   - `else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) { ... }`: 如果发生对端关闭、挂起、或错误事件，可能是客户端连接关闭，调用 `deal_timer` 处理相应的定时器。
   - `else if((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN)) { ... }`: 如果是管道产生的事件，表示有信号到达，调用 `dealwithsignal` 处理信号。
   - `else if(events[i].events & EPOLLIN) { ... }`: 如果是读事件，调用 `dealwithread` 处理读事件。
   - `else if(events[i].events & EPOLLOUT) { ... }`: 如果是写事件，调用 `dealwithwrite` 处理写事件。
6. `if(timeout) { ... }`: 如果发生超时，调用 `utils.timer_handler()` 处理定时器事件，并记录日志。

这段代码组成了服务器的主事件循环，通过 epoll 监听事件并调用相应的处理函数来处理新的连接、信号、读事件、写事件以及定时器事件。

整个WebServer参考https://github.com/vatica/TinyWebSever-CPP