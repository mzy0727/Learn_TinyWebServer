#ifndef LST_TIMER_H
#define LST_TIMER_H

#include <unistd.h>         // 定义了一些常用的符号常量、类型、函数，提供对 POSIX 操作系统 API 的访问
#include <signal.h>         // 定义了信号处理的相关函数、宏和信号编号
#include <sys/types.h>      // 定义了一些系统调用使用的基本数据类型
#include <sys/epoll.h>      // 提供了 epoll I/O 事件通知的支持
#include <fcntl.h>          // 提供对文件控制操作的支持，包括文件描述符的操作
#include <sys/socket.h>     // 定义了套接字相关的数据结构和函数
#include <netinet/in.h>     // 定义了 Internet 地址族相关的数据结构
#include <arpa/inet.h>      // 定义了一些网络地址转换函数
#include <assert.h>         // 包含了一个宏 assert，用于在程序中插入调试断言
#include <sys/stat.h>       // 包含了一些关于文件状态的宏和函数
#include <string.h>         // 包含了一系列有关字符串操作的函数声明
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>       // 提供了内存映射函数的支持
#include <stdarg.h>         // 提供了对变长参数的支持
#include <errno.h>          
#include <sys/wait.h>       // 提供了进程等待和状态报告的函数
#include <sys/uio.h>
#include <time.h>
//#include "../log/log.h"
#include "/media/mzy/learn_TinyWebServer/log/log.h"
// 前向声明
class util_timer;

// 客户端数据结构体
struct client_data{
    sockaddr_in address;    //  客户端地址
    int sockfd;             // 客户socket
    util_timer  *time;      // 定时器
};


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


// 定时器双向链表，按过期时间升序排序
class sort_timer_lst{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);      // 添加定时器
    void adjust_timer(util_timer *timer);   // 调整定时器
    void del_timer(util_timer *timer);      // 删除定时器
    void tick();                            // 定时任务处理函数
    util_timer* get_head();
    util_timer* get_tail();

private:
    void add_timer(util_timer *timer, util_timer *lst_head); // 辅助添加函数
    util_timer *head;       // 链表头
    util_timer *tail;       // 链表尾
};


// 通用类
class Utils{
public:
    Utils(){}
    ~Utils(){}

    // 静态函数避免this指针
    static void sig_handler(int sig);

    void init(int timeslot);
    int setnoblocking(int fd);
    void addfd(int epollfd,int fd, bool one_shot, int TRIGMode);
    void addsig(int sig, void(*handler)(int),bool restart = true);
    void timer_handler();
    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;       // 本地套接字
    static int u_epollfd;       // epoll句柄
    sort_timer_lst m_timer_lst; // 定时器链表
    int m_timeslot;             // 定时时间
};

void cb_func(client_data *user_data);
#endif