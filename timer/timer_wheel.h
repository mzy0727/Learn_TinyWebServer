#ifndef TIMER_WHEEL_H
#define TIMER_WHEEL_H

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
// 前向声明
class tw_timer;

// 用户数据结构
struct client_data{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];  // 存储数据缓冲区
    tw_timer* timer;
};

// 基于时间轮的定时器类
class tw_timer{
public:
    tw_timer(int rot, int ts)
    :next(nullptr),prev(nullptr),rotation(rot),time_slot(ts){}
public:
    int rotation;                   // 记录定时器在时间轮转多少圈后生效
    int time_slot;                  // 记录定时器属于哪一个槽
    void (*cb_func)(client_data*);  // 回调函数
    client_data *user_data;         // 指向用户的指针
    tw_timer* next;                 //指向上一个定时器
    tw_timer* prev;                 //指向下一个定时器
};

// 时间轮类：管理定时器
class timer_wheel{
public:
    timer_wheel():cur_slot(0){
        for(int i = 0; i < N; ++i){
            slots[i] = nullptr;     //每个槽点头节点初始化为空
        }
    }
    ~timer_wheel(){
        for(int i = 0; i < N; i++){
            tw_timer* tmp = slots[i];
            while(tmp != nullptr){
                slots[i] = tmp->next;
                delete tmp; // 遍历每个槽销毁new分配在堆中的定时器
                tmp = slots[i];
            }
        }
    }
    // 根据 timeout 创建一个定时器, 并插入时间轮, 
    // 返回创建的定时器指针, 用户拿到指针后再加入对应的用户数据
    tw_timer* add_timer(int timeout){   // 添加新的定时器，插入到合适的槽中
        if(timeout < 0){    // 时间错误
            return nullptr;
        }
        // 计算定时器在多少个 SI 时间后触发
        int ticks = 0;
        if(timeout < TI){   // 小于每个槽点间隔时间，则为1
            ticks = 1;
        }else{
            ticks = timeout / TI;   // 相对当前位置的槽数
        }

        int rotation = ticks / N;   // 记录多少圈后生效

        int ts = (cur_slot + (ticks % N)) % N;  // 确定插入槽点位置
        tw_timer* timer = new tw_timer(rotation,ts);    // 根据位置和圈数，插入对应的槽中
        if(slots[ts] == nullptr){ // 所在槽头节点为空，直接插入
            printf( "add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot );
            slots[ts] = timer;
        }else{  // 头插法
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
            printf( "add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot );
        }
        return timer;   // 返回含有时间信息和所在槽位置的定时器
    }

    // 删除目标定时器, 当轮子的槽较多时, 复杂度接近 O(n)
    void del_timer(tw_timer* timer){    // 从时间轮上删除定时器
        if(timer == nullptr){
            return ;
        }
        int ts = timer->time_slot;  // 找到所在槽索引
        // 删除操作
        if(timer == slots[ts]){     // 头节点特殊处理
            slots[ts] = slots[ts]->next;
            if(slots[ts] != nullptr){
                slots[ts]->prev = nullptr;
            }
            delete timer;
        }else{      // 其他节点, 双链表的删除操作
            timer->prev->next = timer->next;
            if(timer->next != nullptr){
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }

    // 心跳函数，每次跳完之后轮子向前走一个槽
    void tick(){
        tw_timer *tmp = slots[cur_slot];
        printf( "current slot is %d\n", cur_slot );

        // 遍历当前槽所指向的链表
        while (tmp != nullptr)
        {
            printf( "tick the timer once\n" );

            if(tmp->rotation > 0){
                tmp->rotation--;
                tmp = tmp->next;
            }else{  // 处理到期的定时器
                // 回调
                tmp->cb_func(tmp->user_data);
                if(tmp == slots[cur_slot]){
                    printf( "delete header in cur_slot\n" );
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if(slots[cur_slot] != nullptr){
                        slots[cur_slot]->prev = nullptr;
                    }
                    tmp = slots[cur_slot];
                }else{
                    printf( "delete ordinary node in cur_slot\n" );
                    tmp->prev->next = tmp->next;
                    if(tmp->next != nullptr){
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;  // 轮子往后走一个槽
    }
private:
    static const int N = 60;    // 时间轮的槽数
    static const int TI = 1;    // 时间轮的槽间隔
    tw_timer* slots[N];         // 每个槽的头指针，指向定时器链表的头节点
    int cur_slot;               // 当前时间轮指向的槽
};

#endif