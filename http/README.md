# 从零实现WebServer之HTTP请求类



## http请求类

```c++
class http_conn{
public:
    // 文件名称长度
    static const int FILENAME_LEN = 200;
    // 读缓冲大小
    static const int READ_BUFFER_SIZE = 2048;
    // 写缓冲大小
    static const int WRITE_BUFFER_SIZE = 1024;
    // 请求方法
    enum METHOD{
        GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH
    };
    // 主状态机状态
    enum CHECK_STATE{
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    // 报文解析结果
    enum HTTP_CODE{
        NO_REQUEST = 0, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION
    };
    // 从状态机状态
    enum LINE_STATUS{
        LINE_OK = 0, LINE_BAD, LINE_OPEN
    };

public:
    http_conn(){}
    ~http_conn(){}

public:
    // 初始化连接
    void init(int sockfd, const sockaddr_in &addr, const char *, int, int, string user, string passwd, string sqlname);
    // 关闭连接
    void close_conn(bool real_close=true);
    void process();
    // 读取客户端全部数据
    bool read_once();
    // 写响应报文
    bool write();
    sockaddr_in* get_address(){
        return &m_address;
    }
    // 初始化数据库读取表
    void initmysql_result(connection_pool *connPool);
    int timer_flag;     // reactor是否处理数据
    int improv;         // reactor是否处理失败

private:
    void init();
    // 从m_read_buf读取，处理请求报文
    HTTP_CODE process_read();
    // 向m_write_buf写入响应报文
    bool process_write(HTTP_CODE ret);
    // 主状态机解析请求行数据
    HTTP_CODE parse_request_line(char *text);
    // 主状态机解析请求头数据
    HTTP_CODE parse_headers(char *text);
    // 主状态机解析请求内容
    HTTP_CODE parse_content(char *text);
    // 生成响应报文
    HTTP_CODE do_request();
    // 获得未解读数据位置
    //m_start_line是行在buffer中的起始位置，将该位置后面的数据赋给text
    //此时从状态机已提前将一行的末尾字符\r\n变为\0\0，所以text可以直接取出完整的行进行解析
    char* get_line(){return m_read_buf + m_start_line;};
    // 从状态机读取一行
    LINE_STATUS parse_line();
    void unmap();

    // 生成具体响应报文
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;       // epoll事件表
    static int m_user_count;    // 客户数量
    MYSQL *mysql;               // 数据库连接
    int m_state;                // reactor区分读写任务，0读，1写

private:
    int m_sockfd;               // 客户socket
    sockaddr_in m_address;      // 客户地址
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;             // 已读数据结尾
    int m_checked_idx;          // 解析进行处
    int m_start_line;           // 解析开始处
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;            // 已写数据结尾
    CHECK_STATE m_check_state;  // 主状态机状态
    METHOD m_method;            // 请求方法

    // 解析请求报文变量
    char m_real_file[FILENAME_LEN]; // 实际文件路径
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;          // 是否为长连接

    char *m_file_address;   // 服务器上文件指针
    struct stat m_file_stat;// 文件信息结构体
    struct iovec m_iv[2];   // 向量元素
    int m_iv_count;         // 向量元素个数
    int cgi;                // 是否启用POST
    char *m_string;
    uint32_t bytes_to_send;      // 剩余发送字节
    uint32_t bytes_have_send;    // 已发送字节
    const char *doc_root;        // 资源根目录

    map<string, string> m_users;    // 用户表
    int m_TRIGMode;         // ET模式
    int m_close_log;        // 是否关闭日志

    char sql_user[100];     // 数据库用户名
    char sql_passwd[100];   // 数据库密码
    char sql_name[100];     // 数据库名
};

#endif
```

主要成员变量和枚举：

- `FILENAME_LEN`：文件名长度常量，设为200。
- `READ_BUFFER_SIZE`：读缓冲大小常量，设为2048。
- `WRITE_BUFFER_SIZE`：写缓冲大小常量，设为1024。
- `METHOD`：枚举类型，表示HTTP请求方法，包括GET、POST等。
- `CHECK_STATE`：枚举类型，表示主状态机的三个可能状态：解析请求行、解析请求头、解析请求内容。
- `HTTP_CODE`：枚举类型，表示报文解析结果，包括无请求、GET请求、错误请求等。
- `LINE_STATUS`：枚举类型，表示从状态机的三个可能状态：行正常、行错误、行未完成。

主要成员函数：

- `init`：初始化连接。
- `close_conn`：关闭连接。
- `process`：处理HTTP请求。
- `read_once`：一次性读取客户端全部数据。
- `write`：写响应报文。
- `get_address`：获取客户端地址。
- `initmysql_result`：初始化数据库读取表。
- `parse_request_line`：解析请求行数据。
- `parse_headers`：解析请求头数据。
- `parse_content`：解析请求内容。
- `do_request`：生成响应报文。
- `get_line`：获得未解读数据位置。
- `parse_line`：从状态机读取一行。
- `unmap`：解除映射关系。
- `add_response`：生成具体响应报文。
- `add_content`：添加响应内容。
- `add_status_line`：添加响应状态行。
- `add_headers`：添加响应头。
- `add_content_type`：添加响应内容类型。
- `add_content_length`：添加响应内容长度。
- `add_linger`：添加连接状态。
- `add_blank_line`：添加空行。

静态成员：

- `m_epollfd`：epoll事件表。
- `m_user_count`：客户数量。

### 获取用户表数据 initmysql_result

这段代码是在初始化阶段从MySQL数据库中获取用户表数据，并将用户名和密码存储在内存中的`map<string, string> users`中。同时，使用了`mutexlocker`进行表的互斥锁操作，确保在多线程环境下对用户表的访问是安全的。

```c++
mutexlocker m_lock;         // 表互斥锁
map<string, string> users;  // 内存用户表

void http_conn::initmysql_result(connection_pool *connPool){
    // 从数据库连接池取一个连接
    MYSQL *mysql = nullptr;
    connectionRAII mysqlcon(&mysql, connPool);

    // 在user表中检索
    if(mysql_query(mysql, "SELECT username, password FROM user")){
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    // 结果集的列数
    // int num_fields = mysql_num_fields(result);

    // 所有字段结构的数组
    // MYSQL_FIELD *fields = mysql_fetch_field(result);

    // 将用户名和密码存入map中
    while(MYSQL_ROW row = mysql_fetch_row(result)){
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}mutexlocker m_lock;         // 表互斥锁
map<string, string> users;  // 内存用户表

void http_conn::initmysql_result(connection_pool *connPool){
    // 从数据库连接池取一个连接
    MYSQL *mysql = nullptr;
    connectionRAII mysqlcon(&mysql, connPool);

    // 在user表中检索
    if(mysql_query(mysql, "SELECT username, password FROM user")){
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    // 结果集的列数
    // int num_fields = mysql_num_fields(result);

    // 所有字段结构的数组
    // MYSQL_FIELD *fields = mysql_fetch_field(result);

    // 将用户名和密码存入map中
    while(MYSQL_ROW row = mysql_fetch_row(result)){
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}
```

1. `mutexlocker m_lock;`：定义了一个名为`m_lock`的`mutexlocker`对象，用于在多线程环境中对用户表进行互斥操作。
2. `map<string, string> users;`：定义了一个名为`users`的`map`，用于存储从数据库中读取的用户名和密码。
3. `void http_conn::initmysql_result(connection_pool *connPool)`：这是`http_conn`类的成员函数，用于从MySQL数据库中获取用户表数据。
   - `connectionRAII mysqlcon(&mysql, connPool);`：创建了一个`connectionRAII`对象，通过该对象获取了一个数据库连接。
   - `if(mysql_query(mysql, "SELECT username, password FROM user"))`：执行了一个SQL查询语句，从`user`表中选择用户名和密码。
   - `MYSQL_RES *result = mysql_store_result(mysql);`：将查询结果存储在`result`中。
   - `while(MYSQL_ROW row = mysql_fetch_row(result))`：遍历查询结果的每一行，将用户名和密码存入`map<string, string> users`中。

在这个过程中，由于涉及到对共享数据的读写操作，使用了互斥锁来确保线程安全。这是因为在多线程环境中，多个线程可能同时访问和修改`users`表，为了防止数据不一致和竞争条件，使用互斥锁是一种常见的做法。

### 设置非阻塞 setnonblocking

这段代码定义了一个函数`setnonblocking`，用于将给定的文件描述符设置为非阻塞模式。

```c++
// 对文件描述符设置非阻塞
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
```

1. `int setnonblocking(int fd)`：这是一个函数，接受一个文件描述符`fd`作为参数，返回一个整数值。
2. `int old_option = fcntl(fd, F_GETFL);`：使用`fcntl`系统调用获取当前文件描述符`fd`的文件状态标志。
3. `int new_option = old_option | O_NONBLOCK;`：将`O_NONBLOCK`标志与原有的文件状态标志进行按位或操作，设置文件描述符为非阻塞模式。
4. `fcntl(fd, F_SETFL, new_option);`：使用`fcntl`系统调用将新的文件状态标志设置回文件描述符，使文件描述符变为非阻塞模式。
5. `return old_option;`：返回原有的文件状态标志，方便之后需要的情况下恢复文件描述符的状态。

这种设置非阻塞模式的操作通常用于套接字和其他I/O操作，以便在进行读写时不会被阻塞，能够更好地适应异步I/O的编程模型。

### 注册读事件 addfd

这段代码定义了一个函数`addfd`，用于向内核事件表注册文件描述符的读事件。在ET（边缘触发）模式下，可以选择开启EPOLLONESHOT标志，表示这个文件描述符只能被一个线程处理一次。

```c++
// 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode){
    epoll_event event;
    event.data.fd = fd;

    if(TRIGMode == 1)
        // 读事件、ET模式、对方断开连接
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if(one_shot)
        event.events |= EPOLLONESHOT;

    // 注册事件
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}
```

1. `void addfd(int epollfd, int fd, bool one_shot, int TRIGMode)`：这是一个函数，接受四个参数，分别是`epollfd`（内核事件表的文件描述符）、`fd`（要注册的文件描述符）、`one_shot`（是否开启EPOLLONESHOT标志）、`TRIGMode`（触发模式，这里使用了一个`TRIGMode`参数，值为1表示使用ET模式）。

2. `epoll_event event;`：创建一个`epoll_event`结构体对象，用于描述事件。

3. `event.data.fd = fd;`：设置`event`结构体中的文件描述符字段。

4. `if(TRIGMode == 1)`：判断是否使用ET模式，如果是，设置事件为EPOLLIN（可读事件）、EPOLLET（边缘触发）、EPOLLRDHUP（对方断开连接）。

   如果不是ET模式，仅设置事件为EPOLLIN和EPOLLRDHUP。

5. `if(one_shot)`：判断是否开启EPOLLONESHOT标志，如果是，将`event`结构体中的事件字段加上EPOLLONESHOT。

6. `epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);`：将文件描述符`fd`添加到内核事件表中，并指定对应的事件。

7. `setnonblocking(fd);`：调用之前定义的`setnonblocking`函数，将文件描述符设置为非阻塞模式。

这个函数的目的是将文件描述符注册到内核事件表中，以便后续可以通过epoll来监听这个文件描述符的事件。在ET模式下，EPOLLONESHOT标志的使用表明了一次触发的事件只会被一个线程处理一次，需要重新注册。

### 删除描述符 removefd

这段代码定义了一个函数`removefd`，用于从内核事件表中删除文件描述符，并关闭该文件描述符。

```c++
// 从内核事件表删除描述符，关闭描述符
void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}
```

1. `void removefd(int epollfd, int fd)`：这是一个函数，接受两个参数，分别是`epollfd`（内核事件表的文件描述符）和`fd`（要删除的文件描述符）。
2. `epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);`：调用`epoll_ctl`系统调用，从内核事件表中删除文件描述符`fd`。
3. `close(fd);`：关闭文件描述符，释放相应的资源。

这个函数的目的是在某些情况下，例如文件描述符对应的连接已经关闭，需要从内核事件表中移除该文件描述符，并释放相关资源，以防止不必要的事件触发。

### 重置 modfd

这段代码定义了一个函数`modfd`，用于修改已注册在内核事件表中的文件描述符的事件，将其重置为`EPOLLONESHOT`。这个操作通常在使用边缘触发（ET）模式时，表示一次事件只触发一次，需要重新注册。

```c++
// 将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev, int TRIGMode){
    epoll_event event;
    event.data.fd = fd;

    if(TRIGMode == 1)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
```

1. `void modfd(int epollfd, int fd, int ev, int TRIGMode)`：这是一个函数，接受四个参数，分别是`epollfd`（内核事件表的文件描述符）、`fd`（要修改的文件描述符）、`ev`（新的事件）、`TRIGMode`（触发模式，这里使用了一个`TRIGMode`参数，值为1表示使用ET模式）。

2. `epoll_event event;`：创建一个`epoll_event`结构体对象，用于描述事件。

3. `event.data.fd = fd;`：设置`event`结构体中的文件描述符字段。

4. `if(TRIGMode == 1)`：判断是否使用ET模式，如果是，设置事件为传入的`ev`，并加上EPOLLET（边缘触发）、EPOLLONESHOT（一次触发一次）和EPOLLRDHUP（对方断开连接）标志。

   如果不是ET模式，设置事件为传入的`ev`，并加上EPOLLONESHOT和EPOLLRDHUP标志。

5. `epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);`：调用`epoll_ctl`系统调用，将修改后的事件重新注册到内核事件表中。

这个函数的目的是在某些情况下，例如需要重新监听的事件，将已注册的文件描述符重新设置为EPOLLONESHOT，以确保下一次事件触发时需要重新注册。

### 关闭一个客户连接 close_conn

这部分代码是`http_conn`类的成员函数`close_conn`的实现。主要功能是关闭一个客户连接，并在需要真正关闭时，从内核事件表中移除相应的文件描述符，同时更新类的静态成员`m_user_count`，表示当前的客户数量。

```c++
int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

// 关闭一个客户连接
void http_conn::close_conn(bool real_close){
    if(real_close && (m_sockfd != -1)){
        printf("close %d\n", m_sockfd);
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}
```

1. `void http_conn::close_conn(bool real_close)`：这是一个成员函数，接受一个布尔参数`real_close`，表示是否真正关闭连接。
2. `if(real_close && (m_sockfd != -1))`：判断是否真正关闭连接，并且确保`m_sockfd`不为-1。如果条件成立，执行以下操作：
   - `printf("close %d\n", m_sockfd);`：打印关闭的客户端的文件描述符。
   - `removefd(m_epollfd, m_sockfd);`：调用之前定义的`removefd`函数，从内核事件表中删除该文件描述符，同时关闭文件描述符。
   - `m_sockfd = -1;`：将类的成员变量`m_sockfd`设置为-1，表示连接已关闭。
   - `m_user_count--;`：减少静态成员`m_user_count`，表示当前的客户数量减少了一个。

这个函数的主要作用是关闭客户连接，释放相关资源，并更新当前客户数量。在一些情况下，可能需要保持文件描述符在内核事件表中注册，待后续操作。但在这里，采取了一次性关闭文件描述符并从内核事件表中移除的方式。

### 初始化新接受的连接 init

这段代码是`http_conn`类的成员函数`init`的实现，用于初始化新接受的连接。

```c++
// 初始化新接受的连接
void http_conn::init(){
    mysql = nullptr;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf, 0, READ_BUFFER_SIZE);
    memset(m_write_buf, 0, WRITE_BUFFER_SIZE);
    memset(m_real_file, 0, FILENAME_LEN);
}
```

1. `mysql = nullptr;`：将`mysql`指针设置为nullptr，表示暂时没有数据库连接。
2. `bytes_to_send = 0;`：将`bytes_to_send`设置为0，表示待发送的字节数为0。
3. `bytes_have_send = 0;`：将`bytes_have_send`设置为0，表示已发送的字节数为0。
4. `m_check_state = CHECK_STATE_REQUESTLINE;`：将主状态机状态设置为`CHECK_STATE_REQUESTLINE`，表示正在解析请求行。
5. `m_linger = false;`：将`m_linger`设置为false，表示不使用长连接。
6. `m_method = GET;`：将请求方法设置为GET。
7. `m_url = 0;`、`m_version = 0;`、`m_host = 0;`：将与请求相关的指针设置为0，表示未初始化。
8. `m_start_line = 0;`：将解析开始位置设置为0。
9. `m_checked_idx = 0;`：将解析进行位置设置为0。
10. `m_read_idx = 0;`：将已读数据结尾位置设置为0。
11. `m_write_idx = 0;`：将已写数据结尾位置设置为0。
12. `cgi = 0;`：将CGI标志设置为0。
13. `m_state = 0;`：将Reactor区分读写任务的标志设置为0，表示当前状态为读。
14. `timer_flag = 0;`：将`timer_flag`设置为0，表示Reactor是否处理数据。
15. `improv = 0;`：将`improv`设置为0，表示Reactor是否处理失败。
16. `memset(m_read_buf, 0, READ_BUFFER_SIZE);`：使用`memset`函数将`m_read_buf`数组中的所有元素设置为0。
17. `memset(m_write_buf, 0, WRITE_BUFFER_SIZE);`：使用`memset`函数将`m_write_buf`数组中的所有元素设置为0。
18. `memset(m_real_file, 0, FILENAME_LEN);`：使用`memset`函数将`m_real_file`数组中的所有元素设置为0。

这个函数的作用是将`http_conn`类的各个成员变量重置为初始状态，以便处理新接受的连接。

### 初始化连接 init

这段代码是`http_conn`类的成员函数`init`的另一种实现，用于初始化连接。该函数接受多个参数，包括套接字描述符（`sockfd`）、客户端地址信息（`addr`）、根目录（`root`）、触发模式（`TRIGMode`）、是否关闭日志（`close_log`）、数据库用户名（`user`）、数据库密码（`passwd`）以及数据库名（`sqlname`）。

```c++
// 初始化连接
void http_conn::init(int sockfd, const sockaddr_in &addr, const char *root, int TRIGMode, 
                     int close_log, string user, string passwd, string sqlname){
    m_sockfd = sockfd;
    m_address = addr;

    addfd(m_epollfd, sockfd, true, m_TRIGMode);
    m_user_count++;

    doc_root = root;
    m_TRIGMode = TRIGMode;
    m_close_log = close_log;

    strcpy(sql_user, user.c_str());
    strcpy(sql_passwd, passwd.c_str());
    strcpy(sql_name, sqlname.c_str());

    init();
}
```

1. `m_sockfd = sockfd;`：将成员变量`m_sockfd`设置为传入的套接字描述符。
2. `m_address = addr;`：将成员变量`m_address`设置为传入的客户端地址信息。
3. `addfd(m_epollfd, sockfd, true, m_TRIGMode);`：调用之前定义的`addfd`函数，将套接字描述符注册到内核事件表中，同时设置EPOLLONESHOT标志。
4. `m_user_count++;`：增加静态成员变量`m_user_count`，表示当前的客户数量增加了一个。
5. `doc_root = root;`：将成员变量`doc_root`设置为传入的根目录。
6. `m_TRIGMode = TRIGMode;`：将成员变量`m_TRIGMode`设置为传入的触发模式。
7. `m_close_log = close_log;`：将成员变量`m_close_log`设置为传入的是否关闭日志标志。
8. `strcpy(sql_user, user.c_str());`、`strcpy(sql_passwd, passwd.c_str());`、`strcpy(sql_name, sqlname.c_str());`：将数据库用户名、密码和数据库名拷贝到相应的成员变量中。
9. `init();`：调用之前定义的`init`函数，将`http_conn`类的各个成员变量重置为初始状态。

这个函数的作用是初始化`http_conn`类的连接信息，包括套接字描述符、地址信息、根目录、触发模式等，并将连接注册到内核事件表中。

### 分析一行内容 parse_line

这段代码是`http_conn`类的成员函数`parse_line`的实现，用于从状态机中分析一行内容。

```c++
// 从状态机，用于分析一行内容
http_conn::LINE_STATUS http_conn::parse_line(){
    char temp;
    //m_read_idx指向缓冲区m_read_buf的数据末尾的下一个字节
    //m_checked_idx指向从状态机当前正在分析的字节
    for(; m_checked_idx < m_read_idx; ++m_checked_idx){
        temp = m_read_buf[m_checked_idx];
        // 可能读取到完整行
        if(temp == '\r'){
            // 不完整
            if((m_checked_idx + 1) == m_read_idx){
                return LINE_OPEN;
            }
            // 完整，\r\n替换为\0\0
            else if(m_read_buf[m_checked_idx + 1] == '\n'){
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            // 格式错误
            return LINE_BAD;
        }
        // 可能读取到完整行
        else if(temp == '\n'){
            // 完整，\r\n替换为\0\0
            if(m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r'){
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    // 继续接收
    return LINE_OPEN;
}
```

1. `char temp;`：定义一个临时变量`temp`，用于存储当前分析的字符。
2. `for(; m_checked_idx < m_read_idx; ++m_checked_idx)`：循环遍历已经读取的数据缓冲区`m_read_buf`中的内容，从`m_checked_idx`开始，直到当前已读取的数据结尾`m_read_idx`。
3. `temp = m_read_buf[m_checked_idx];`：获取当前分析的字符。
4. `if(temp == '\r')`：判断当前字符是否为回车符（`\r`），如果是，表示可能读取到了一行。
   - `if((m_checked_idx + 1) == m_read_idx)`：如果回车符后面没有字符了，表示不完整的行，返回`LINE_OPEN`，继续接收数据。
   - `else if(m_read_buf[m_checked_idx + 1] == '\n')`：如果回车符后面是换行符（`\n`），表示读取到了完整的一行，将`\r\n`替换为`\0\0`，并返回`LINE_OK`。
   - `else`：如果回车符后面不是换行符，表示行格式错误，返回`LINE_BAD`。
5. `else if(temp == '\n')`：如果当前字符是换行符（`\n`），表示可能读取到了一行。
   - `if(m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')`：如果换行符前面是回车符，表示读取到了完整的一行，将`\r\n`替换为`\0\0`，并返回`LINE_OK`。
   - `else`：如果换行符前面不是回车符，表示行格式错误，返回`LINE_BAD`。
6. 如果以上条件都不满足，继续循环，继续接收数据，返回`LINE_OPEN`。

这个函数的主要目的是在已接收的数据中分析一行内容，判断是否读取到了完整的一行，并进行相应的处理。

### 循环读取客户数据 read_once

这段代码是`http_conn`类的成员函数`read_once`的实现，用于循环读取客户端数据，直到无数据可读或对方关闭连接。

```c++
// 循环读取客户数据，直到无数据可读或对方关闭连接
bool http_conn::read_once(){
    if(m_read_idx >= READ_BUFFER_SIZE){
        return false;
    }
    int bytes_read = 0;

    // LT模式
    if(m_TRIGMode == 0){
        // 读取数据到缓冲区
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytes_read;

        if(bytes_read <= 0){
            return false;
        }
        return true;
    }
    // ET模式
    else{
        while(true){
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            // 无数据可读
            if(bytes_read == -1){
                // 非阻塞、连接正常
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            // 另一端已关闭
            else if(bytes_read == 0){
                return false;
            }
            m_read_idx += bytes_read;
        }
        return true;
    }
}
```

1. `if(m_read_idx >= READ_BUFFER_SIZE)`：判断当前已读取的数据是否已经达到缓冲区的最大容量，如果是，返回`false`，表示缓冲区已满，无法再读取更多数据。
2. `int bytes_read = 0;`：定义一个变量`bytes_read`，用于存储每次读取的字节数。
3. `if(m_TRIGMode == 0)`：如果是LT模式（level-triggered模式）。
   - `bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);`：调用`recv`函数从套接字中读取数据到缓冲区中。
   - `m_read_idx += bytes_read;`：更新已读取的数据结尾位置。
   - `if(bytes_read <= 0)`：如果读取的字节数小于等于0，表示对方关闭了连接或发生了错误，返回`false`。
   - 返回`true`，表示读取数据成功。
4. `else`：如果是ET模式（edge-triggered模式）。
   - `while(true)`：进入无限循环，持续读取数据。
     - `bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);`：调用`recv`函数从套接字中读取数据到缓冲区中。
     - `if(bytes_read == -1)`：如果读取的字节数为-1，表示无数据可读。
       - `if(errno == EAGAIN || errno == EWOULDBLOCK)`：如果是非阻塞模式，并且连接正常，退出循环，表示当前没有更多数据可读。
         - `break;`
       - 返回`false`，表示发生了错误或对方关闭了连接。
     - `else if(bytes_read == 0)`：如果读取的字节数为0，表示对方已关闭了连接，返回`false`。
     - `m_read_idx += bytes_read;`：更新已读取的数据结尾位置。
   - 返回`true`，表示读取数据成功。

这个函数的主要目的是在LT或ET模式下，从套接字中循环读取数据，直到无数据可读或对方关闭连接。在ET模式下，通过循环读取保证将所有的可读数据都读取出来。

### 解析HTTP请求行 parse_request_line

这段代码是`http_conn`类的成员函数`parse_request_line`的实现，用于解析HTTP请求行，获取请求方法、目标URL及HTTP版本号。

```c++
// 解析HTTP请求行，获得请求方法、目标url及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text){
    // 寻找空格和\t位置
    m_url = strpbrk(text, " \t");
    if(!m_url){
        return BAD_REQUEST;
    }
    // 将该位置改为\0，方便取出
    *m_url++ = '\0';
    // 取出请求方法并比较
    char *method = text;
    if(strcasecmp(method, "GET") == 0)
        m_method = GET;
    else if(strcasecmp(method, "POST") == 0){
        m_method = POST;
        cgi = 1;
    }
    else return BAD_REQUEST;

    // 跳过空格和\t
    m_url += strspn(m_url, " \t");
    // 再寻找空格和\t
    m_version = strpbrk(m_url, " \t");
    if(!m_version)
        return BAD_REQUEST;
    *m_version++ = '\0';
    // 跳过空格和\t
    m_version += strspn(m_version, " \t");

    // 仅支持HTTP1.1
    if(strcasecmp(m_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    // 去除http://
    if(strncasecmp(m_url, "http://", 7) == 0){
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    // 去除https://
    if(strncasecmp(m_url, "https://", 8) == 0){
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if(!m_url || m_url[0] != '/')
        return BAD_REQUEST;
    // url为/，显示主页
    if(strlen(m_url) == 1)
        strcat(m_url, "judge.html");

    // 主状态机状态修改为处理请求头
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

```

1. `m_url = strpbrk(text, " \t");`：在请求行中寻找空格和制表符的位置，返回第一个匹配的位置，赋值给`m_url`。如果没有找到，返回`BAD_REQUEST`，表示请求行格式错误。
2. `if(!m_url)`：如果`m_url`为空，返回`BAD_REQUEST`，表示请求行格式错误。
3. `*m_url++ = '\0';`：将`m_url`指向的位置改为`\0`，即将请求方法和URL分隔开。
4. `char *method = text;`：将`method`指针指向请求行的开始位置，即请求方法的起始位置。
5. `if(strcasecmp(method, "GET") == 0)`：比较请求方法是否为"GET"，如果是，将`m_method`设置为`GET`；否则，如果是"POST"，将`m_method`设置为`POST`，并设置`cgi`标志为1（表示启用CGI）；否则，返回`BAD_REQUEST`，表示请求方法不支持。
6. `m_url += strspn(m_url, " \t");`：跳过空格和制表符，将`m_url`指向下一个非空白字符。
7. `m_version = strpbrk(m_url, " \t");`：在新的位置继续寻找空格和制表符的位置，返回第一个匹配的位置，赋值给`m_version`。如果没有找到，返回`BAD_REQUEST`，表示请求行格式错误。
8. `if(!m_version)`：如果`m_version`为空，返回`BAD_REQUEST`，表示请求行格式错误。
9. `*m_version++ = '\0';`：将`m_version`指向的位置改为`\0`，即将URL和HTTP版本号分隔开。
10. `m_version += strspn(m_version, " \t");`：跳过空格和制表符，将`m_version`指向下一个非空白字符。
11. `if(strcasecmp(m_version, "HTTP/1.1") != 0)`：比较HTTP版本号是否为"HTTP/1.1"，如果不是，返回`BAD_REQUEST`，表示不支持的HTTP版本。
12. `if(strncasecmp(m_url, "http://", 7) == 0)`：如果URL以"http://"开头，去除该部分。
13. `if(strncasecmp(m_url, "https://", 8) == 0)`：如果URL以"https://"开头，去除该部分。
14. `if(!m_url || m_url[0] != '/')`：如果URL为空或者不以'/'开头，返回`BAD_REQUEST`，表示请求行格式错误。
15. `if(strlen(m_url) == 1)`：如果URL长度为1，表示只有'/'，将其拼接为"judge.html"，作为默认的主页。
16. `m_check_state = CHECK_STATE_HEADER;`：将主状态机状态修改为处理请求头。
17. `return NO_REQUEST;`：返回`NO_REQUEST`，表示成功解析请求行。

这个函数的主要目的是解析HTTP请求行，获取请求方法、目标URL及HTTP版本号，并进行相应的处理。

### 解析HTTP请求头 parse_headers

这段代码是`http_conn`类的成员函数`parse_headers`的实现，用于解析HTTP请求的一个头部信息。

```c++
// 解析HTTP请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text){
    // 首位为\0是空行
    if(text[0] == '\0'){
        if(m_content_length != 0){
            // POST继续解析消息体
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // GET请求解析完成
        return GET_REQUEST;
    }
    // 连接字段
    else if(strncasecmp(text, "connection:", 11) == 0){
        text += 11;
        text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0){
            // 长连接
            m_linger = true;
        }
    }
    // 内容长度字段
    else if(strncasecmp(text, "Content-length:", 15) == 0){
        text += 15;
        text += strspn(text, "\t");
        m_content_length = atol(text);
    }
    // Host字段，请求站点
    else if(strncasecmp(text, "Host:", 5) == 0){
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else{
        LOG_INFO("oop! unknow header: %s", text);
    }
    return NO_REQUEST;
}
```

1. `if(text[0] == '\0')`：判断头部信息的首位是否为`\0`，如果是，表示空行，说明当前请求的头部信息解析完成。
   - `if(m_content_length != 0)`：如果请求方法是POST且有消息体，将主状态机状态修改为处理请求体，返回`NO_REQUEST`。
   - 否则，表示GET请求头部解析完成，返回`GET_REQUEST`。
2. `else if(strncasecmp(text, "connection:", 11) == 0)`：如果头部字段以"Connection:"开头，表示连接字段。
   - `text += 11;`：跳过"Connection:"部分。
   - `text += strspn(text, " \t");`：跳过空格和制表符，指向字段值的开始。
   - `if(strcasecmp(text, "keep-alive") == 0)`：如果字段值为"keep-alive"，表示使用长连接，将`m_linger`标志设置为`true`。
3. `else if(strncasecmp(text, "Content-length:", 15) == 0)`：如果头部字段以"Content-length:"开头，表示内容长度字段。
   - `text += 15;`：跳过"Content-length:"部分。
   - `text += strspn(text, "\t");`：跳过制表符，指向字段值的开始。
   - `m_content_length = atol(text);`：将字段值转换为长整型，表示消息体的长度。
4. `else if(strncasecmp(text, "Host:", 5) == 0)`：如果头部字段以"Host:"开头，表示Host字段，即请求的站点。
   - `text += 5;`：跳过"Host:"部分。
   - `text += strspn(text, " \t");`：跳过空格和制表符，指向字段值的开始。
   - `m_host = text;`：将`m_host`指针指向Host字段的值，即请求的站点。
5. `else`：如果是其他未知的头部字段，输出日志记录。
6. 返回`NO_REQUEST`，表示继续解析后续的头部信息。

这个函数的主要目的是解析HTTP请求的一个头部信息，包括连接字段、内容长度字段和Host字段。在解析过程中，如果发现空行，表示当前请求的头部信息解析完成，根据情况进行处理。

### 判断HTTP请求是否被完全读入 parse_content

这段代码是`http_conn`类的成员函数`parse_content`的实现，用于判断HTTP请求是否已经完全读入消息体。

```c++
// 判断HTTP请求是否被完全读入
http_conn::HTTP_CODE http_conn::parse_content(char *text){
    // 判断buffer中是否读取了消息体
    if(m_read_idx >= (m_content_length + m_checked_idx)){
        // 最后填充\0
        text[m_content_length] = '\0';
        // 获取消息体
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}
```

1. `if(m_read_idx >= (m_content_length + m_checked_idx))`：判断当前已读取的数据长度是否大于等于消息体长度加上已解析的数据位置。
   - `text[m_content_length] = '\0';`：在消息体结束位置填充`\0`，将消息体转换为以`\0`结尾的字符串。
   - `m_string = text;`：将`m_string`指针指向消息体的起始位置，即请求体的内容。
   - 返回`GET_REQUEST`，表示HTTP请求的消息体已经完全读入。
2. 返回`NO_REQUEST`，表示HTTP请求的消息体还未完全读入，需要继续读取。

### 处理接收到的HTTP请求数据 process_read 

这段代码是`http_conn`类的成员函数`process_read`的实现，用于处理接收到的HTTP请求数据。

```c++
http_conn::HTTP_CODE http_conn::process_read()
{
    // 初始化状态
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;

    // 消息体末尾没有字符，POST请求报文不能用从状态机LINE_OK状态判断
    // 加上&& line_status == LINE_OK防止陷入死循环
    while((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK)){
        // 取一行数据
        text = get_line();
        m_start_line = m_checked_idx;
        LOG_INFO("get line: %s", text);

        // 主状态机状态
        switch (m_check_state){
            case CHECK_STATE_REQUESTLINE:{
                ret = parse_request_line(text);
                if(ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER:{
                ret = parse_headers(text);
                if(ret == BAD_REQUEST)
                    return BAD_REQUEST;
                // 完整解析GET请求
                else if(ret == GET_REQUEST)
                    return do_request();
                break;
            }
            case CHECK_STATE_CONTENT:{
                ret = parse_content(text);
                // 完整解析POST请求
                if(ret == GET_REQUEST)
                    return do_request();
                // 完成报文解析，防止进入循环
                line_status = LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}
```

1. `LINE_STATUS line_status = LINE_OK;`：初始化行状态为`LINE_OK`，表示当前行的状态正常。

2. `HTTP_CODE ret = NO_REQUEST;`：初始化HTTP处理状态为`NO_REQUEST`，表示当前没有完整的HTTP请求。

3. `char *text = nullptr;`：初始化字符指针`text`为空指针。

4. ```c++
   while((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
   ```

   ：循环条件，如果当前主状态机状态是解析消息体并且当前行状态是OK，或者解析行的状态是OK，则进入循环。

   - `text = get_line();`：获取一行数据，`get_line()`函数返回指向当前行的指针，并将`m_start_line`设置为当前解析位置。

   - `m_start_line = m_checked_idx;`：更新解析起始位置。

   - `LOG_INFO("get line: %s", text);`：记录日志，输出获取的行数据。

   - ```c++
     switch (m_check_state){
     ```

     ：根据主状态机的状态进行处理。

     - `case CHECK_STATE_REQUESTLINE:`：处理请求行，调用`parse_request_line`函数解析请求行，返回相应的处理结果，如果是`BAD_REQUEST`，返回`BAD_REQUEST`；如果是`GET_REQUEST`，则调用`do_request`函数进行处理。
     - `case CHECK_STATE_HEADER:`：处理请求头，调用`parse_headers`函数解析请求头，返回相应的处理结果，如果是`BAD_REQUEST`，返回`BAD_REQUEST`；如果是`GET_REQUEST`，则调用`do_request`函数进行处理。
     - `case CHECK_STATE_CONTENT:`：处理请求体，调用`parse_content`函数解析请求体，返回相应的处理结果，如果是`GET_REQUEST`，则调用`do_request`函数进行处理；将行状态设置为`LINE_OPEN`，防止进入死循环。
     - `default:`：如果状态无法匹配，返回`INTERNAL_ERROR`，表示内部错误。

5. `return NO_REQUEST;`：最终返回`NO_REQUEST`，表示处理完成，等待下一次读取请求。

### 请求实现 do_request

这段代码是`http_conn`类的成员函数`do_request`的实现，用于处理HTTP请求。

```c++
http_conn::HTTP_CODE http_conn::do_request(){
    // 网站根目录
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);

    // 找到最后一个/的位置
    const char *p = strrchr(m_url, '/');

    // POST请求，实现登录和注册校验
    if(cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3')){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/");
        strcat(m_url_real, m_url + 2);
        strncpy(m_real_file + len, m_url_real, FILENAME_LEN - len - 1);
        free(m_url_real);

        // 将用户名和密码提取出来
        // user=123&password=123
        char name[100], password[100];
        int i;
        for(i = 5; m_string[i] != '&'; ++i)
            name[i - 5] = m_string[i];
        name[i - 5] = '\0';

        int j = 0;
        for(i = i + 10; m_string[i] != '\0'; ++i, ++j)
            password[j] = m_string[i];
        password[j] = '\0';

        if(*(p + 1) == '3'){
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, password) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            if(users.find(name) == users.end()){
                m_lock.lock();
                int res = mysql_query(mysql, sql_insert);
                users.insert(pair<string, string>(name, password));
                m_lock.unlock();

                if(!res)
                    strcpy(m_url, "/log.html");
                else
                    strcpy(m_url, "/registerError.html");
            }
            else
                strcpy(m_url, "/registerError.html");
        }
        //如果是登录，直接判断
        //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
        else if(*(p + 1) == '2')
        {
            if(users.find(name) != users.end() && users[name] == password)
                strcpy(m_url, "/welcome.html");
            else
                strcpy(m_url, "/logError.html");
        }
    }
    // GET请求，跳转到注册页面
    if(*(p + 1) == '0'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/register.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    // GET请求，跳转到登录页面
    else if(*(p + 1) == '1'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/log.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    // POST请求，图片页面
    else if(*(p + 1) == '5'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/picture.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    // POST请求，视频页面
    else if(*(p + 1) == '6'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/video.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    // POST请求，关注页面
    else if(*(p + 1) == '7'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/fans.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else
        // 都不是则直接拼接
        strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);

    // 获取不到文件信息，资源不存在
    if(stat(m_real_file, &m_file_stat) < 0)
        return NO_RESOURCE;

    // 文件是否可读
    if(!(m_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    // 文件是否为目录
    if(S_ISDIR(m_file_stat.st_mode))
        return BAD_REQUEST;

    // 以只读方式打开文件并映射到内存中
    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    // 关闭文件描述符
    close(fd);
    return FILE_REQUEST;
}
```

1. `strcpy(m_real_file, doc_root);`：复制网站根目录到`m_real_file`。

2. `const char *p = strrchr(m_url, '/');`：找到请求URL中最后一个'/'的位置。

3. ```c++
   if(cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3'))
   ```

   ：判断是否是CGI请求（登录或注册）。

   - `char *m_url_real = (char *)malloc(sizeof(char) * 200);`：为`m_url_real`分配内存。
   - 根据请求类型（'2'为登录，'3'为注册），拼接请求的URL到`m_url_real`。
   - 将`m_url_real`拼接到`m_real_file`，得到完整的文件路径。
   - 解析POST请求中的用户名和密码，然后进行相应的处理。

4. ```c++
   else if(*(p + 1) == '0')
   ```

   ：GET请求，跳转到注册页面。

   - `char *m_url_real = (char *)malloc(sizeof(char) * 200);`：为`m_url_real`分配内存。
   - 将注册页面的URL拼接到`m_url_real`，然后将其拼接到`m_real_file`。

5. ```c++
   else if(*(p + 1) == '1')
   ```

   ：GET请求，跳转到登录页面。

   - `char *m_url_real = (char *)malloc(sizeof(char) * 200);`：为`m_url_real`分配内存。
   - 将登录页面的URL拼接到`m_url_real`，然后将其拼接到`m_real_file`。

6. ```c++
   else if(*(p + 1) == '5')
   ```

   ：POST请求，图片页面。

   - 类似地处理图片、视频、关注等不同类型的页面。

7. `else`：都不是上述情况，则直接拼接URL到`m_real_file`。

8. `if(stat(m_real_file, &m_file_stat) < 0)`：获取文件信息，如果失败则返回`NO_RESOURCE`表示资源不存在。

9. `if(!(m_file_stat.st_mode & S_IROTH))`：判断文件是否可读，如果不可读则返回`FORBIDDEN_REQUEST`表示禁止访问。

10. `if(S_ISDIR(m_file_stat.st_mode))`：判断文件是否为目录，如果是目录则返回`BAD_REQUEST`表示请求不合法。

11. `int fd = open(m_real_file, O_RDONLY);`：以只读方式打开文件。

12. `m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);`：将文件映射到内存中。

13. `close(fd);`：关闭文件描述符。

14. `return FILE_REQUEST;`：返回`FILE_REQUEST`，表示文件请求处理完成。

### 关闭文件映射 unmap

这段代码是`http_conn`类的成员函数`unmap`的实现，用于关闭文件映射。

```c++
// 关闭文件映射
void http_conn::unmap(){
    if(m_file_address){
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}
```

1. `if(m_file_address)`：检查`m_file_address`是否非空，即是否已经映射了文件。
2. `munmap(m_file_address, m_file_stat.st_size);`：调用`munmap`函数关闭文件映射，释放映射的内存。
3. `m_file_address = 0;`：将`m_file_address`置为0，表示文件映射已关闭。

### 向客户端写响应 write

这段代码是`http_conn`类的成员函数`write`的实现，主要用于向客户端写响应。

```c++
// 向客户端写响应
bool http_conn::write(){
    int temp = 0;

    // 响应报文为空
    if(bytes_to_send == 0){
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        init();
        return true;
    }

    while(1){
        // 发送响应
        temp = writev(m_sockfd, m_iv, m_iv_count);

        if(temp < 0){
            // 重试，继续监听写事件
            if(errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
                // 不要断开连接
                return true;
            }
            // 断开连接
            unmap();
            return false;
        }

        // 正常发送
        bytes_have_send += temp;
        bytes_to_send -= temp;
        // 第一个元素已经发送完
        if(bytes_have_send >= m_iv[0].iov_len){
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        // 继续发送第一个元素
        else{
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        // 全部发送完毕
        if(bytes_to_send <= 0){
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);

            // 长连接
            if(m_linger){
                init();
                return true;
            }
            else{
                return false;
            }
        }
    }
}
```

1. `if(bytes_to_send == 0)`：如果响应报文为空，表示数据已经发送完毕，可以修改`epoll`事件，然后初始化连接，准备处理下一个请求。

2. `while(1)`：进入一个无限循环，用于循环发送响应数据。

3. `temp = writev(m_sockfd, m_iv, m_iv_count);`：使用`writev`函数向客户端发送响应，`m_iv`是`iovec`结构体数组，指定了发送的数据块和长度。

4. 判断`temp`的返回值：

   - ```c++
     temp < 0
     ```

     ：发生错误，根据错误类型采取相应的处理措施。

     - 如果是 `EAGAIN`，表示当前无法写入更多数据，此时应该修改`epoll`事件监听写事件，等待下一次写事件到来，然后继续循环。
     - 如果是其他错误，可能是连接断开等问题，调用`unmap`关闭文件映射，然后返回`false`表示写入失败。

5. `temp >= 0`：正常发送数据，更新已发送和剩余待发送的字节数。如果第一个`iovec`元素的数据已经发送完毕，将其长度置为0，并更新第二个元素的位置和长度，继续发送。

6. 判断`bytes_to_send`是否小于等于0：

   - 如果是，表示全部数据发送完毕，调用`unmap`关闭文件映射，修改`epoll`事件为监听读事件，并根据长连接标志决定是否初始化连接准备处理下一个请求。
   - 如果不是，继续下一轮循环，继续发送。

### 写响应 add_response

这段代码是`http_conn`类的成员函数`add_response`的实现，主要用于往写缓冲区中添加HTTP响应内容。

```c++
// 写响应
bool http_conn::add_response(const char *format, ...){
    // 写入内容超过buffer长度则报错
    if(m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    
    // 可变参数列表
    va_list arg_list;
    // 初始化列表
    va_start(arg_list, format);

    // 按照format写入缓存
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    // 超出缓存则报错
    if(len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)){
        va_end(arg_list);
        return false;
    }

    // 更新idx
    m_write_idx += len;
    va_end(arg_list);

    LOG_INFO("add response: %s", m_write_buf);
    return true;
}
```

1. `if(m_write_idx >= WRITE_BUFFER_SIZE)`：检查写缓冲区是否已满，如果已满，则表示无法继续添加响应内容，返回`false`。
2. `va_list arg_list;`：声明一个变量参数列表。
3. `va_start(arg_list, format);`：初始化变量参数列表，使其指向参数列表中的第一个可变参数。
4. `int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);`：使用`vsnprintf`函数按照指定的`format`格式将可变参数列表中的内容写入写缓冲区。`vsnprintf`会根据指定的格式将可变参数列表中的内容格式化并写入指定长度的缓冲区。
5. `if(len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))`：检查写入的内容长度是否超过了写缓冲区的剩余空间，如果超过则表示写入过程中发生截断，返回`false`。
6. `m_write_idx += len;`：更新写缓冲区的索引位置，将已写入的长度加到索引上。
7. `va_end(arg_list);`：结束变量参数列表的使用。
8. `LOG_INFO("add response: %s", m_write_buf);`：打印日志，记录添加的响应内容。
9. `return true;`：表示添加响应内容成功。

该函数允许通过格式化字符串和可变参数列表的方式往写缓冲区中添加HTTP响应内容，便于构建响应报文。

### 状态行 add_status_line

这是`http_conn`类的成员函数`add_status_line`的实现，用于向写缓冲区中添加HTTP响应的状态行。

```c++
// 状态行
bool http_conn::add_status_line(int status, const char *title){
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
```

1. `add_response("%s %d %s\r\n", "HTTP/1.1", status, title)`：通过调用`add_response`函数，使用格式化字符串往写缓冲区中添加状态行内容。`"%s %d %s\r\n"`是格式化字符串，其中：
   - `%s` 会被替换为 "HTTP/1.1"
   - `%d` 会被替换为 `status` 参数传递的状态码
   - `%s` 会被替换为 `title` 参数传递的状态码对应的文本描述
   - `\r\n` 表示回车和换行，标识行结束
2. 返回`add_response`的执行结果。如果写缓冲区已满或其他写入错误，可能返回`false`。

该函数主要用于构建HTTP响应的状态行，指定协议版本、状态码和状态码的文本描述。

### 响应报头 add_headers

这是`http_conn`类的成员函数`add_headers`的实现，用于向写缓冲区中添加HTTP响应的报头。

```c++
// 响应报头
bool http_conn::add_headers(int content_len){
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}
```

1. `add_content_length(content_len)`：通过调用`add_content_length`函数，向写缓冲区中添加"Content-Length"字段，表示消息体的长度。`content_len`是传递的消息体长度。
2. `add_linger()`：通过调用`add_linger`函数，向写缓冲区中添加"Connection"字段，表示是否使用持久连接。该函数会根据成员变量`m_linger`的值来确定是"keep-alive"还是"close"。
3. `add_blank_line()`：通过调用`add_blank_line`函数，向写缓冲区中添加一个空行，用于分隔报头和消息体。
4. 返回上述三个函数的执行结果的逻辑与(`&&`)运算结果。如果有任何一个函数返回`false`，整体结果也为`false`。

该函数主要用于构建HTTP响应的报头，包括"Content-Length"字段、"Connection"字段以及一个空行。

### 消息报头 add_content_length

```c++
// 消息报头
bool http_conn::add_content_length(int content_len){
    return add_response("Content-Length:%d\r\n", content_len);
}
```

这是`http_conn`类的成员函数`add_content_length`的实现，用于向写缓冲区中添加HTTP响应的"Content-Length"字段。以下是该函数的解释：

1. `add_response("Content-Length:%d\r\n", content_len)`：通过调用`add_response`函数，向写缓冲区中添加"Content-Length"字段，指定消息体的长度为`content_len`。
2. 返回上述函数的执行结果。

该函数主要用于构建HTTP响应的报头中的"Content-Length"字段，指明消息体的长度。

### Content-Type add_content_type

这是`http_conn`类的成员函数`add_content_type`的实现，用于向写缓冲区中添加HTTP响应的`Content-Type`字段。

```c++
bool http_conn::add_content_type(){
    return add_response("Content-Type:%s\r\n", "text/html");
}
```

1. `add_response("Content-Type:%s\r\n", "text/html")`：通过调用`add_response`函数，向写缓冲区中添加"Content-Type"字段，指定消息体的类型为"text/html"，表示HTML文档。
2. 返回上述函数的执行结果。

该函数主要用于构建HTTP响应的报头中的"Content-Type"字段，指明消息体的类型。在这里，它指定消息体的类型为HTML文档。

### add_linger

这是`http_conn`类的成员函数`add_linger`的实现，用于向写缓冲区中添加HTTP响应的"Connection"字段。以下是该函数的解释：

```c++
bool http_conn::add_linger(){
    return add_response("Connection:%s\r\n", (m_linger == true) ? "keep-alive" : "close");
}
```

1. `add_response("Connection:%s\r\n", (m_linger == true) ? "keep-alive" : "close")`：通过调用`add_response`函数，向写缓冲区中添加"Connection"字段。如果`m_linger`成员变量为`true`，表示使用长连接，将"keep-alive"添加到字段中；否则，表示使用短连接，将"close"添加到字段中。
2. 返回上述函数的执行结果。

该函数主要用于构建HTTP响应的报头中的"Connection"字段，指明连接的持久性。如果`m_linger`为`true`，则使用长连接；否则，使用短连接。

### 空行 add_blank_line

```c++
// 空行
bool http_conn::add_blank_line(){
    return add_response("%s", "\r\n");
}这是`http_conn`类的成员函数`add_blank_line`的实现，用于向写缓冲区中添加一个空行，即`\r\n`，表示HTTP响应头部的结束。以下是该函数的解释：
```

1. `add_response("%s", "\r\n")`：通过调用`add_response`函数，向写缓冲区中添加一个空行。
2. 返回上述函数的执行结果。

该函数主要用于构建HTTP响应头的结束标志，以便与响应的实体主体（如果有的话）分隔开来。

### 响应正文 add_content

```c++
// 响应正文
bool http_conn::add_content(const char *content){
    return add_response("%s", content);
}
```

这是`http_conn`类的成员函数`add_content`的实现，用于向写缓冲区中添加HTTP响应正文内容。以下是该函数的解释：

1. `add_response("%s", content)`：通过调用`add_response`函数，向写缓冲区中添加传入的`content`参数，即HTTP响应的实体主体内容。
2. 返回上述函数的执行结果。

该函数主要用于向HTTP响应中添加实体主体的内容，例如 HTML 内容、图片、视频等。

### 向缓冲区写响应 process_write

```c++
// 向缓冲区写响应
bool http_conn::process_write(HTTP_CODE ret){
    switch (ret){
        case INTERNAL_ERROR:{
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if(!add_content(error_500_form))
                return false;
            break;
        }
        case BAD_REQUEST:{
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if(!add_content(error_404_form))
                return false;
            break;
        }
        case FORBIDDEN_REQUEST:{
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if(!add_content(error_403_form))
                return false;
            break;
        }
        case FILE_REQUEST:{
            add_status_line(200, ok_200_title);
            if(m_file_stat.st_size != 0){
                add_headers(m_file_stat.st_size);
                // 第一个元素指向响应报文写缓冲
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                // 第二个元素指向mmap返回的文件指针
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                return true;
            }
            // 资源大小为0则返回空白html
            else{
                const char *ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if(!add_content(ok_string))
                    return false;
            }
        }
        default:
            return false;
    }
    // 除FILE_REQUEST外只指向响应报文缓冲
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}
```

`process_write` 函数主要用于根据不同的 `HTTP_CODE` 处理响应。具体来说，根据传入的 `HTTP_CODE`，生成相应的 HTTP 响应报文。以下是对每个情况的解释：

1. `INTERNAL_ERROR`：生成 500 Internal Server Error 错误响应。设置状态行、响应头、实体主体，并将相关信息写入缓冲区。
2. `BAD_REQUEST`：生成 404 Not Found 错误响应。设置状态行、响应头、实体主体，并将相关信息写入缓冲区。
3. `FORBIDDEN_REQUEST`：生成 403 Forbidden 错误响应。设置状态行、响应头、实体主体，并将相关信息写入缓冲区。
4. `FILE_REQUEST`：处理正常的文件请求。设置状态行、响应头，并将文件内容映射到写缓冲区中。这里使用 `writev` 函数进行数据的直接写入。
5. 其他情况：返回 `false`。

最后，根据生成的响应，设置写缓冲区的数据，并准备发送给客户端。

### http处理 process

```c++
// http处理
void http_conn::process(){
    HTTP_CODE read_ret = process_read();
    // 请求不完整，继续注册读事件
    if(read_ret == NO_REQUEST){
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        return;
    }
    bool write_ret = process_write(read_ret);
    if(!write_ret){
        close_conn();
    }
    // 准备好写缓冲，加入监听可写事件
    modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
}
```

`process` 函数用于处理 HTTP 请求。首先调用 `process_read` 函数进行请求的解析，根据返回的 `HTTP_CODE` 判断请求是否完整。如果请求不完整，则继续注册监听读事件。

如果请求完整，接着调用 `process_write` 函数生成相应的响应。如果 `process_write` 返回 `false`，则关闭连接。否则，准备好写缓冲，加入监听可写事件。

这个函数的逻辑是基于事件驱动的异步非阻塞模型，通过监听和处理事件来实现高并发。