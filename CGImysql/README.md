# 从零实现WebServer之数据库连接池类

## 数据库连接池类

### 流程图

![数据库连接池](https://glf-1309623969.cos.ap-shanghai.myqcloud.com/img/%E6%95%B0%E6%8D%AE%E5%BA%93%E8%BF%9E%E6%8E%A5%E6%B1%A0.svg)

![数据库连接池和RAII流程图](https://glf-1309623969.cos.ap-shanghai.myqcloud.com/img/image-20231229150130892.png)

### 获取实例 get_instance

```c++
 // 单例模式
static connection_pool* get_instance(){
    static connection_pool connPool;
    return &connPool;
}
```

这是一个典型的单例模式实现，通过在静态成员函数 `get_instance` 中创建 `static` 局部变量 `connPool`，确保了在程序的生命周期内只有一个实例被创建。这种方式被称为懒汉式单例模式，因为实例在第一次调用 `get_instance` 时被创建。 在 C++11 及以上标准中，对于静态局部变量的初始化是线程安全的，因此这种实现在多线程环境下是安全的。在早期的 C++ 标准中，可能需要考虑额外的线程安全性措施。当程序结束时，`connPool` 对象将在全局静态存储区自动销毁，确保了释放资源。

### 构造函数 connection_pool

```c++
connection_pool::connection_pool(){
    m_cur_conn = 0;
    m_free_conn = 0;
}
```

在构造函数 `connection_pool::connection_pool()` 中，初始化了 `m_cur_conn` 和 `m_free_conn` 两个成员变量。这样的初始化是很常见的，通常用于确保在对象创建时，这些成员变量处于一个已知的初始状态。

- `m_cur_conn` 用于记录当前已经使用的数据库连接数。
- `m_free_conn` 用于记录当前空闲的数据库连接数。

通过将它们初始化为零，确保了在对象创建时，这两个计数器的初始值为零。接下来，在程序运行时，通过 `get_connection` 获取连接时，`m_cur_conn` 会递增，而在 `release_connection` 释放连接时，`m_cur_conn` 会递减，同时 `m_free_conn` 也会相应地增加或减少。这样的设计方案有助于在连接池中管理数据库连接的数量，以及追踪当前连接的使用情况。

### 析构函数 ~connection_pool

```c++
connection_pool::~connection_pool(){
    list<MYSQL*>::iterator iter;
    for(iter = conn_list.begin(); iter != conn_list.end(); ++iter){
        MYSQL * conn = *iter;
        mysql_close(conn);
    }
}
```

在 `connection_pool` 类的析构函数 `connection_pool::~connection_pool()` 中，对连接池中的 `MySQL` 连接进行了关闭。

具体地说，它遍历了 `conn_list` 列表，关闭了每个 MySQL 连接。这是一个良好的实践，因为在对象销毁时，应该释放所有的资源，包括打开的数据库连接。通过在析构函数中完成这个工作，确保了在对象生命周期结束时，相关资源能够被正确释放，从而避免了潜在的资源泄漏问题。

需要注意的是，`MySQL `连接的关闭不仅仅是释放连接对象本身，还包括释放底层的资源，确保在数据库连接不再需要时，相关资源能够被正确释放。

### 初始化连接池 init

```c++
// 初始化连接池
void connection_pool::init(string url, string user, string password, string database_name, int port, int max_conn, int close_log){
    m_url = url;
    m_port = port;
    m_user = user;
    m_password = password;
    m_database_name = database_name;
    m_close_log = close_log;

    // 构造连接
    for(int i = 0; i < max_conn; ++i){
        // 初始化一个mysql连接的实例对象，MYSQL* mysql_init(MYSQL *mysql);
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if(conn == nullptr){
            LOG_ERROR("MySQL init Error");
            exit(1);
        }

        // 与数据库引擎建立连接
        conn = mysql_real_connect(conn,url.c_str(),user.c_str(),password.c_str(),database_name.c_str(),port,nullptr,0);
        if(conn == nullptr){
            LOG_ERROR("MySQL real connect Error");
            exit(1);
        }

        // 添加到连接链表
        conn_list.push_back(conn);
        ++m_free_conn;
    }

    // 创建信号量
    reserve = semaphore(m_free_conn);
    m_max_conn = m_free_conn;

}
```

在 `connection_pool` 类的初始化函数 `connection_pool::init` 中，进行了数据库连接池的初始化工作。下面是对该函数的一些主要步骤的解释：

1. **初始化成员变量：** 将函数参数中传递的各种连接信息（如主机地址、用户名、密码等）赋值给类的成员变量。

   ```c++
   m_url = url;
   m_port = port;
   m_user = user;
   m_password = password;
   m_database_name = database_name;
   m_close_log = close_log;
   ```

2. **构造连接：** 使用 `mysql_init` 函数初始化一个 `MySQL` 连接的实例对象。

   ```c++
   MYSQL *conn = nullptr;
   conn = mysql_init(conn);
   ```

   如果初始化失败，记录错误日志并退出程序。

3. **建立与数据库引擎的连接：** 使用 `mysql_real_connect` 函数与数据库引擎建立连接。

   ```c++
   conn = mysql_real_connect(conn, url.c_str(), user.c_str(), password.c_str(), database_name.c_str(), port, nullptr, 0);
   ```

   如果连接失败，记录错误日志并退出程序。

4. **添加到连接链表：** 将成功建立连接的 `MySQL` 对象添加到连接链表中，并增加空闲连接数。

   ```c++
   conn_list.push_back(conn);
   ++m_free_conn;
   ```

5. **创建信号量：** 使用 `semaphore` 类创建一个信号量对象 `reserve`，初始值为空闲连接数。

   ```c++
   reserve = semaphore(m_free_conn);
   ```

6. **设置最大连接数：** 将当前的空闲连接数作为最大连接数。

   ```c++
   m_max_conn = m_free_conn;
   ```

该函数的目标是初始化数据库连接池，包括创建连接、建立与数据库引擎的连接、管理连接链表和信号量等。这是一个典型的连接池初始化流程。

### 获取可用连接 get_connection

```c++
// 有请求时，从数据库连接池返回一个可用连接
MYSQL* connection_pool::get_connection(){
    // 无空闲连接
    if(conn_list.size() == 0){
        return nullptr;
    }
    MYSQL *conn = nullptr;
    // 等待空闲连接
    reserve.wait();
    // 加互斥锁
    lock.lock();
    // 取连接池第一个连接
    conn = conn_list.front();
    conn_list.pop_front();

    --m_free_conn;
    ++m_cur_conn;
    lock.unlock();
    return conn;
}
```

`connection_pool::get_connection` 函数用于在有请求时从数据库连接池返回一个可用连接。下面是对该函数的主要步骤解释：

1. **检查连接池是否有空闲连接：** 如果连接池中没有空闲连接（`conn_list.size() == 0`），则返回 `nullptr` 表示没有可用连接。

   ```c++
   if(conn_list.size() == 0){
       return nullptr;
   }
   ```

2. **等待空闲连接：** 调用信号量的 `wait` 函数等待有可用连接为止。

   ```c++
   reserve.wait();
   ```

   这里使用了信号量 `reserve` 来表示有多少个可用连接。如果 `reserve` 的值为零，表示没有可用连接，该函数将会被阻塞。

3. **获取连接：** 在获取连接之前，使用互斥锁保护临界区，以确保线程安全。从连接链表的头部获取一个连接，并从链表中移除。

   ```c++
   lock.lock();
   conn = conn_list.front();
   conn_list.pop_front();
   ```

4. **更新连接计数：** 减少空闲连接数，增加当前已使用连接数。

   ```c++
   --m_free_conn;
   ++m_cur_conn;
   lock.unlock();
   ```

5. **返回连接：** 返回获取到的连接。

   ```c++
   return conn;
   ```

{% note success %}

该函数在有请求时从连接池中获取一个可用连接，确保线程安全地管理连接池状态。

在 `connection_pool::init` 函数中，初始化的过程是在单线程环境下进行的，因此不需要加锁。在初始化过程中，不会有其他线程同时尝试修改连接池的状态，因此不存在竞争条件。

而在 `connection_pool::get_connection` 函数中，涉及到获取连接、更新连接池状态等操作，这些操作在多线程环境下可能会被多个线程同时调用。所有需要使用互斥锁进行保护。

{% endnote %}

### 释放连接 release_connection

```c++
// 释放当前使用的连接，成功返回true
bool connection_pool::release_connection(MYSQL *conn){
    if(conn == nullptr){
        return false;
    }
    lock.lock();
    conn_list.push_back(conn);
    ++m_free_conn;
    --m_cur_conn;
    lock.unlock();

    reserve.post();
    return true;
}
```

在 `connection_pool::release_connection` 函数中，通过互斥锁 `lock` 对连接池的状态进行了修改，确保在多线程环境下对连接池的操作是互斥的。这是因为在释放连接的过程中，会涉及到对连接池中的连接列表、空闲连接数、当前使用连接数等状态的修改，而这些修改是不可分割的操作。

以下是函数中的主要操作：

1. **加锁：** 使用互斥锁 `lock.lock()`，将当前线程锁住，防止其他线程同时修改连接池状态。
2. **连接释放：** 将当前使用的连接 `conn` 放回连接池的连接列表 `conn_list` 中，同时更新空闲连接数 `m_free_conn` 和当前使用连接数 `m_cur_conn`。
3. **解锁：** 使用互斥锁 `lock.unlock()`，释放锁，允许其他线程访问连接池。
4. **信号量增加：** 调用 `reserve.post()`，将信号量的计数增加，表示有一个空闲连接可用。

这样设计确保了在多线程环境下对连接池状态的修改是线程安全的。如果不使用互斥锁进行保护，可能会导致多个线程同时修改连接池状态而出现问题。

### 当前空闲连接数 get_freeconn

```c++
// 当前空闲连接数
int connection_pool::get_freeconn(){
    return this->m_free_conn;
}
```

## 资源获取即初始化

{% note success %}

`connectionRAII` 类是一个简单的资源获取即初始化（RAII）类。在构造函数中获取资源（数据库连接），在析构函数中释放这些资源。这个类的主要目的是通过构造函数获得资源（数据库连接），并在对象销毁时通过析构函数释放这些资源。使用这个类可以确保在任何情况下都能正确地释放数据库连接，**避免资源泄漏**。这是 C++ 中一种常见的资源管理方式，称为 RAII 设计模式。

{% endnote %}

### 获取连接 connectionRAII

```c++
// 从连接池获取一个数据库连接
connectionRAII::connectionRAII(MYSQL **conn, connection_pool *connPool){
    *conn = connPool->get_connection();
    connRAII = *conn;
    poolRAII = connPool;
}
```

`connectionRAII` 类的构造函数是一个资源获取即初始化的操作，通过该构造函数可以获取一个数据库连接并初始化 `connRAII` 和 `poolRAII` 成员变量。

具体步骤如下：

1. **获取连接：** 调用 `connPool->get_connection()` 从数据库连接池中获取一个连接，并将连接指针赋值给 `*conn`，这是通过传入的参数来返回连接的。
2. **初始化成员变量：** 将获取的连接赋值给 `connRAII`，表示当前对象拥有该数据库连接。
3. **保存连接池指针：** 将传入的 `connPool` 赋值给 `poolRAII`，表示当前对象持有该连接所属的连接池的指针。

这样，在 `connectionRAII` 对象构造时，就完成了数据库连接的获取，并且通过 `connRAII` 持有了这个连接，同时也保存了连接所属的连接池的指针。这符合资源获取即初始化的 RAII 设计思想。

### 释放连接 ~connectionRAII

```c++
// 释放持有的数据库连接
connectionRAII::~connectionRAII(){
    poolRAII->release_connection(connRAII);
}
```

`connectionRAII` 类的析构函数实现了资源的释放操作。在析构函数中，它调用了 `poolRAII->release_connection(connRAII)` 来释放持有的数据库连接。

**释放连接：** 调用 `poolRAII->release_connection(connRAII)`，将持有的数据库连接 `connRAII` 释放回连接池。这个操作会使连接池的空闲连接数量增加，同时当前对象持有的连接数量减少。

通过这样的设计，当 `connectionRAII` 对象生命周期结束时（例如，超出了其作用域），它会自动释放持有的数据库连接，确保连接得到有效的回收和重用。这符合 RAII 设计模式，简化了资源管理的代码。

以下部分参考：[C++多线程连接池MySQL (foryouos.cn)](https://www.blog.foryouos.cn/computer-science/cpp/course-3/cpp多线程连接池/)

![连接池URML](https://glf-1309623969.cos.ap-shanghai.myqcloud.com/img/0)

## C++ 封装 MySQL API

###  构造函数 (`MySqlConnect::MySqlConnect`)：

```c++
MySqlConnect::MySqlConnect() {
    m_conn = mysql_init(nullptr);
    mysql_set_character_set(m_conn, "utf8");
}
```

- **功能：** 初始化`m_conn`，即MySQL连接对象。
- 详解：
  - 使用`mysql_init`初始化MySQL连接对象。
  - 通过`mysql_set_character_set`设置字符集为"utf8"，以支持UTF-8编码。

### 析构函数 (`MySqlConnect::~MySqlConnect`)：

```c++
MySqlConnect::~MySqlConnect() {
    if (m_conn != nullptr) {
        mysql_close(m_conn);
    }
    freeResult();
}
```

- **功能：** 关闭MySQL连接，释放相关资源。
- 详解：
  - 使用`mysql_close`关闭MySQL连接。
  - 调用`freeResult`释放执行查询时产生的结果集。

### 释放结果集 (`MySqlConnect::freeResult`)：

```c++
void MySqlConnect::freeResult() {
    if (m_result) {
        mysql_free_result(m_result);
        m_result = nullptr;
    }
}
```

- **功能：** 释放结果集占用的内存。
- 详解：
  - 使用`mysql_free_result`释放结果集占用的内存。
  - 将`m_result`置为`nullptr`，确保不再指向已释放的内存。

### 连接数据库 (`MySqlConnect::connect`)：

```c++
bool MySqlConnect::connect(string user, string passwd, string dbName, string ip, unsigned short port) {
    MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    return ptr != nullptr;
}
```

- **功能：** 连接到MySQL数据库。
- 详解：
  - 使用`mysql_real_connect`进行实际的数据库连接。
  - 返回连接是否成功的布尔值。

### 执行更新操作 (`MySqlConnect::update`)：

```c++
bool MySqlConnect::update(string sql) {
    if (mysql_query(m_conn, sql.c_str())) {
        return false;
    }
    return true;
}
```

- **功能：** 执行更新操作（插入、更新、删除等）的SQL语句。
- 详解：
  - 使用`mysql_query`执行SQL语句。
  - 返回执行是否成功的布尔值。

### 执行查询操作 (`MySqlConnect::query`)：

```c++
bool MySqlConnect::query(string sql) {
    freeResult();
    if (mysql_query(m_conn, sql.c_str())) {
        return false;
    }
    m_result = mysql_store_result(m_conn);
    return true;
}
```

- **功能：** 执行查询操作的SQL语句。
- 详解：
  - 在执行查询之前通过`freeResult`释放之前的结果集。
  - 使用`mysql_query`执行SQL语句。
  - 使用`mysql_store_result`将查询结果存储在`m_result`中。
  - 返回执行是否成功的布尔值。

### 遍历查询结果 (`MySqlConnect::next`)：

```c++
bool MySqlConnect::next() {
    if (m_result != nullptr) {
        m_row = mysql_fetch_row(m_result);
        return true;
    }
    return false;
}
```

- **功能：** 移动到结果集中的下一行。
- 详解：
  - 使用`mysql_fetch_row`获取结果集中的下一行数据。
  - 返回是否成功移动到下一行的布尔值。

### 获取结果集中的字段值 (`MySqlConnect::value`)：

```c++
string MySqlConnect::value(int index) {
    int row_num = mysql_num_fields(m_result);
    if (index >= row_num || index < 0) {
        return string();
    }
    char* val = m_row[index];
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return string(val, length);
}
```

- **功能：** 获取当前行中指定列的值。
- 详解：
  - 使用`mysql_num_fields`获取结果集中的列数。
  - 如果指定的列索引超出范围，返回空字符串。
  - 使用`mysql_fetch_lengths`获取列的长度。
  - 返回当前行中指定列的字符串值。

### 事务操作 (`MySqlConnect::transaction`, `MySqlConnect::commit`, `MySqlConnect::rollback`)：

```c++
bool MySqlConnect::transaction() {
    return mysql_autocommit(m_conn, false);
}

bool MySqlConnect::commit() {
    return mysql_commit(m_conn);
}

bool MySqlConnect::rollback() {
    return mysql_rollback(m_conn);
}
```

- **功能：** 执行事务操作（开启事务、提交、回滚）。
- 详解：
  - `transaction`方法用于开启事务，关闭自动提交。
  - `commit`方法用于提交事务。
  - `rollback`方法用于回滚事务。
  - 返回执行是否成功的布尔值。

### MySQLConnect.h

```c++
#pragma once
#ifndef MYSQLCONNECT_H
#define MYSQLCONNECT_H
#include <iostream>
#include <mysql.h>
using namespace std;
class MySqlConnect
{
private:
	// 什么时候调用释放结果集
	//1, 析构函数 2，可能会对数据库进行多次查询，每次查询一次都会得到结果集，查询是清空掉上次的结果集
	void freeResult(); // 释放结果集
	MYSQL* m_conn = nullptr; // 保存 MySQL 初始化的私有成员
	MYSQL_RES* m_result = nullptr; // 报错结果集
	MYSQL_ROW m_row = nullptr; // 保存着当前字段的所有列的数值
public:
	// 初始化数据库连接
	MySqlConnect();
	// 释放数据库连接
	~MySqlConnect();
	// 连接数据库，使用默认端口可省略端口书写
	bool connect(string user, string passwd,string dbName,string ip,unsigned short port = 3306 );
	// 更新数据库 (插入，更新，删除)，传递字符串

	bool update(string sql);
	// 查询数据库，单词 query: 查询
	bool query(string sql);

	// 遍历查询得到的结果集，每调一次，从结果集中取出一条数据

	bool next(); 
	// 得到结果集中的字段值，取记录里面字段方法
	string value(int index); 
	// 事务操作，关闭自动提交
	bool transaction();
	// 提交事务
	bool commit();
	// 事务回滚；
	bool rollback();
};
#endif // !MYSQLCONNECT_H
```

### MySQLConnect.cpp

```c++
#include "MySQLConnect.h"

void MySqlConnect::freeResult()
{
	if (m_result)
	{
		mysql_free_result(m_result);
		m_result = nullptr;
	}
}

MySqlConnect::MySqlConnect()
{
	m_conn = mysql_init(nullptr);
	mysql_set_character_set(m_conn, "utf8");
}

MySqlConnect::~MySqlConnect()
{
	if (m_conn != nullptr)
	{
		mysql_close(m_conn);
	}
	freeResult();
}

bool MySqlConnect::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
	//ip 传入为 string，使用.str 将 ip 转为 char * 类型
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
	// 连接成功返回 true
	// 如果连接成功返回 TRUE，失败返回 FALSE
	return ptr!=nullptr;
}

bool MySqlConnect::update(string sql)
{
	//query 执行成功返回 0
	if (mysql_query(m_conn, sql.c_str()))
	{
		return false;
	};

	return true;
}

bool
MySqlConnect::query(string sql)
{
	freeResult();
	//query 执行成功返回 0
	if (mysql_query(m_conn, sql.c_str()))
	{
		return false;
	};
	m_result = mysql_store_result(m_conn);
	return true;
}

bool MySqlConnect::next()
{
	// 如果结果集为空则没有必要遍历
	if (m_result != nullptr)
	{
		// 保存着当前字段的所有列的数值
		m_row = mysql_fetch_row(m_result);
		return true;
	}
	return false;
}

string MySqlConnect::value(int index)
{ 
	// 表示列的数量
	int row_num = mysql_num_fields(m_result); // 函数得到结果集中的列数
	// 如果查询的的 index 列大于总列，或小于 0，是错误的
	if (index >= row_num || index < 0)
	{
		return string();
	}
	char* val = m_row[index]; // 若为二进制数据，中间是有 "\0" 的
	unsigned long length = mysql_fetch_lengths(m_result)[index];
	return string(val,length); // 传入 length 就不会以 "\0" 为结束符，而是通过长度把对应的字符转换为 string 类型
}

bool MySqlConnect::transaction()
{
	return mysql_autocommit(m_conn,false); // 函数返回值本身就是 bool 类型
}

bool MySqlConnect::commit()
{
	return mysql_commit(m_conn);// 提交
}

bool MySqlConnect::rollback()
{
	return mysql_rollback(m_conn);//bool 类型，函数成功返回 TRUE，失败返回 FALSE
}
```

## 数据库连接池：包含封装MySQL API

### 构造函数 (`ConnectionPool::ConnectionPool`)：

```c++
ConnectionPool::ConnectionPool() {
    if (!parseJsonFile()) {
        cout << "数据库连接失败" << endl;
        return;
    }
    for (int i = 0; i < m_minSize; ++i) {
        if (m_connectionQ.size() < m_maxSize) {
            MySqlConnect* conn = new MySqlConnect;
            conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
            m_connectionQ.push(conn);
        } else {
            cout << "当前连接数量已超过允许的最大连接数" << endl;
            break;
        }
    }
    thread producer(&ConnectionPool::produceConnection, this);
    thread recycler(&ConnectionPool::recycleConnection, this);
    producer.detach();
    recycler.detach();
}
```

- **功能：** 构造函数，初始化连接池。
- 详解：
  - 调用`parseJsonFile`加载配置信息。
  - 初始化连接池中的连接，保证达到最小连接数。
  - 创建两个线程，一个用于生成新连接(`produceConnection`)，一个用于回收连接(`recycleConnection`)。
  - 将生成连接线程和回收连接线程分离。

### 析构函数 (`ConnectionPool::~ConnectionPool`)：

```c++
ConnectionPool::~ConnectionPool() {
    while (!m_connectionQ.empty()) {
        MySqlConnect* conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}
```

- **功能：** 析构函数，释放连接池中的连接。
- 详解：
  - 循环弹出连接队列中的连接，并释放内存。

### 解析JSON文件 (`ConnectionPool::parseJsonFile`)：

```c++
bool ConnectionPool::parseJsonFile() {
    try {
        ifstream ifs("dbconf.json");
        Reader rd;
        Value root;
        rd.parse(ifs, root);
        if (root.isObject()) {
            m_ip = root["ip"].asString();
            m_port = root["port"].asInt();
            m_user = root["userName"].asString();
            m_passwd = root["password"].asString();
            m_dbName = root["dbName"].asString();
            m_minSize = root["minSize"].asInt();
            m_maxSize = root["maxSize"].asInt();
            m_maxIdleTime = root["maxIdleTime"].asInt();
            m_timeout = root["timeout"].asInt();
            return true;
        }
        throw("读取连接数据库json失败！");
        return false;
    } catch (exception& e) {
        cout << e.what() << endl;
        return false;
    }
}
```

- **功能：** 解析JSON配置文件。
- 详解：
  - 打开`dbconf.json`文件，使用`Reader`解析JSON。
  - 从JSON中提取数据库连接的相关信息。

### 生产连接 (`ConnectionPool::produceConnection`)：

```c++
void ConnectionPool::produceConnection() {
    while (true) {
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() >= m_minSize) {
            m_cond.wait(locker);
        }
        addConnection();
        m_cond.notify_all();
    }
}
```

- **功能：** 生成新的数据库连接。
- 详解：
  - 在连接池连接数不足时，生成新的数据库连接。
  - 使用`addConnection`添加连接到连接池。
  - 唤醒等待的消费者。

### 回收连接 (`ConnectionPool::recycleConnection`)：

```c++
void ConnectionPool::recycleConnection() {
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(500));
        lock_guard<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() > m_minSize) {
            MySqlConnect* conn = m_connectionQ.front();
            if (conn->getAliveTime() >= m_maxIdleTime) {
                m_connectionQ.pop();
                delete conn;
            } else {
                break;
            }
        }
    }
}
```

- **功能：** 回收空闲连接。
- 详解：
  - 每隔一定时间检查连接池中的连接，将超过最大空闲时间的连接释放。
  - 通过`sleep_for`休息一段时间。

### 添加连接 (`ConnectionPool::addConnection`)：

```c++
void ConnectionPool::addConnection() {
    MySqlConnect* conn = new MySqlConnect;
    conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    conn->refreshAliveTime();
    m_connectionQ.push(conn);
}
```

- **功能：** 添加新连接到连接池。
- 详解：
  - 创建新的`MySqlConnect`对象，连接数据库。
  - 刷新连接的起始空闲时间。
  - 将连接添加到连接池队列中。

### 获取连接 (`ConnectionPool::getConnection`)：

```c++
shared_ptr<MySqlConnect> ConnectionPool::getConnection() {
    unique_lock<mutex> locker(m_mutexQ);
    while (m_connectionQ.empty()) {
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout))) {
            if (m_connectionQ.empty()) {
                continue;
            }
        }
    }
    shared_ptr<MySqlConnect> connptr(m_connectionQ.front(), [this](MySqlConnect* conn) {
        lock_guard<mutex> locker(m_mutexQ);
        conn->refreshAliveTime();
        m_connectionQ.push(conn);
    });
    m_connectionQ.pop();
    m_cond.notify_all();
    return connptr;
}
```

- **功能：** 获取数据库连接。
- 详解：
  - 使用`unique_lock`上锁，保证线程安全。
  - 当连接池为空时，等待可用连接或超时。
  - 获取连接并返回其`shared_ptr`，使用lambda表达式在连接释放时刷新连接的起始空闲时间。
  - 唤醒等待的生产者线程。

###  ConnectionPool.h

```c++
#pragma once
// 连接池头文件
#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#include<queue>
#include "MySQLConnect.h"
#include <mutex> //C++ 独占的互斥锁
#include <condition_variable> // 引用 C++ 条件变量
using namespace std;
// 连接池
class ConnectionPool
{
public:
	// 静态实例，通过静态方法获得唯一的单例对象
	static ConnectionPool* getConnectPool();
	// 删除掉构造函数
	ConnectionPool(const ConnectionPool& obj) = delete;
	// 移动赋值函数重载，删除掉，防止对象的复制
	ConnectionPool& operator =(const ConnectionPool& obj)= delete;
	// 获取连接时返回一个可用的连接，返回共享的智能指针
	shared_ptr<MySqlConnect> getConnection();
	// 析构函数
	~ConnectionPool();
private:
	ConnectionPool();
	// 解析 JSON 文件的函数
	bool paraseJsonFile();
	// 用来生产数据库连接
	void produceConnection();
	
	// 用来销毁数据库连接 回收数据库连接
	void recycleConnection();
	// 添加连接
	void addConnection();

	// 数据库相关信息
	// 通过加载配置文件 Json，访问用户指定的数据库
	// 数据库 ip
	string m_ip; 
	// 数据库用户
	string m_user;
	// 数据库密码
	string m_passwd;
	// 数据库名称
	string m_dbName;
	// 数据库访问端口
	unsigned short m_port;
	// 设置连接上限
	int m_minSize;
	// 设置连接的上限
	int m_maxSize;

	// 设置线程等待最大时长，单位毫秒
	// 超时时长
	int m_timeout; 
	// 最大空闲时长单位毫秒
	int m_maxIdleTime;

	// 存储若干数据库连接队列
	queue<MySqlConnect*> m_connectionQ;
	// 设置互斥锁
	mutex m_mutexQ;
	// 设置条件变量
	condition_variable m_cond;

};
#endif // !CONNECTIONPOOL_H
```

### ConnectionPool.cpp

```c++
#include "ConnectionPool.h"
#include <json/json.h>
#include <fstream>
#include <mysql.h>
#include <thread> // 加载多线程
using namespace Json;
// 静态函数
ConnectionPool* ConnectionPool::getConnectPool()
{
    static ConnectionPool pool; // 静态局部对象，不管后面调用多少次，得到的都是同一块内存地址

    return &pool;
}
// 打开数据库信息文件，并判断是否读取到相关信息
bool ConnectionPool::paraseJsonFile()
{
    try
    {
        ifstream ifs("dbconf.json");
        Reader rd;
        Value root;
        rd.parse(ifs, root);
        if (root.isObject())
        {
            m_ip = root["ip"].asString();
            m_port = root["port"].asInt();
            m_user = root["userName"].asString();
            m_passwd = root["password"].asString();
            m_dbName = root["dbName"].asString();
            m_minSize = root["minSize"].asInt();
            m_maxSize = root["maxSize"].asInt();
            m_maxIdleTime = root["maxIdlTime"].asInt();
            m_timeout = root["timeout"].asInt();
            return true;
        }
        throw("读取连接数据库json失败！");
        return false;
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }
    
    
}
// 子线程对应的任务函数，生成新的可用连接
void ConnectionPool::produceConnection()
{
    //
    while (true) 
    {
        // 判断当前连接池是否够用
        //uniuqe 模版类，mutex 互斥锁类型 locker 对象管理
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() >= m_minSize)
        {
            // 阻塞条件变量
            m_cond.wait(locker);
        }
        // 生产一个数据库连接
        addConnection();
        // 调用对应的唤醒函数，唤醒的所有消费者
        m_cond.notify_all();
    }
}

// 当空闲的链接数量过多
void ConnectionPool::recycleConnection()
{
    while (true)
    {
        // 休息一段时间，每隔一秒种，进行一次检测
        this_thread::sleep_for(chrono::milliseconds(500));
        // 进行加锁
        lock_guard<mutex>locker(m_mutexQ);
        // 当大于最小连接数
        while (m_connectionQ.size() > m_minSize)
        {
            // 先进后出
            MySqlConnect* conn = m_connectionQ.front(); // 取出队头元素
            // 判断队头元素存活时长是不是大于指定的最长存活时长
            if (conn->getAliveTime() >= m_maxIdleTime)
            {
                m_connectionQ.pop(); // 将队头的链接销毁
                delete conn;
            }
            else
            {
                break;
            }
        }
    }
}
void ConnectionPool::addConnection()
{
    MySqlConnect* conn = new MySqlConnect;
    conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    // 数据库连接之后就开始记录时间错
    conn->refreshAliveTime();
    m_connectionQ.push(conn);
}
shared_ptr<MySqlConnect> ConnectionPool::getConnection()
{
    // 封装互斥锁，保证线程安全
    unique_lock<mutex> locker(m_mutexQ);
    // 检查是否有可用的连接，如果没有阻塞一会
    while (m_connectionQ.empty())
    {
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout)))
 
        {
            if (m_connectionQ.empty())
            {
                continue;
            }
        }
    }
    shared_ptr<MySqlConnect> connptr(m_connectionQ.front(), [this](MySqlConnect* conn) 
        {
            // 加锁
            lock_guard<mutex> locker(m_mutexQ);
           // 刷新起始空闲时间
            conn->refreshAliveTime();   
            m_connectionQ.push(conn);
        });
    m_connectionQ.pop();

    m_cond.notify_all();// 唤醒生产者

    return connptr;
}
// 线程池析构函数
ConnectionPool::~ConnectionPool()
{
    while (m_connectionQ.empty())
    {
        MySqlConnect* conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}
// 构造函数的实现
ConnectionPool::ConnectionPool()
{
    // 加载配置文件
    if (!paraseJsonFile())
    {
        cout << "数据库连接失败" << endl;
        return;
    }
    // 初始化配置连接数
    for (int i = 0; i < m_minSize; ++i)  // 连接数
    { 
        // 如果队列总数小于最大数量
        if (m_connectionQ.size() < m_maxSize)
        {
            // 实例化对象
            MySqlConnect* conn = new MySqlConnect;
            // 链接数据库
            conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);

            m_connectionQ.push(conn);
        }
        // 当连接总数大于允许连接的最大数量
        else
        {
            cout << "当前连接数量以超过允许连接的总数" << endl;
            break;
        }
    }
    // 当前实例对象 this 指针，单例模式，
    thread producer(&ConnectionPool::produceConnection,this); // 生成线程池的连接
    thread recycler(&ConnectionPool::recycleConnection,this); // 有没有需要销毁的连接
    /*
    将 producer 线程与当前线程分离，使得它们可以独立执行，
    */
    // 主线程和子线程分离
    producer.detach();
    recycler.detach();
}
```

