#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <iostream>
#include <netinet/in.h>
#include <time.h>
#include <exception>

using namespace std;

#define BUFFER_SIZE 64

// 前向声明
class heap_timer;
// 用户数据结构
struct client_data{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];  // 存储数据缓冲区
    heap_timer* timer;
};
// 定时器
class heap_timer {
public:
    // 构造函数，接受一个延时参数 delay，计算过期时间
    heap_timer(int delay) {
        expire = time(NULL) + delay;
    }

public:
    time_t expire;               // 定时器的过期时间
    void (*cb_func)(client_data*);  // 回调函数指针，用于定时器到期时执行某个操作
    client_data* user_data;      // 指向用户数据结构的指针
};
// 时间堆
class timer_heap{
public:
    // 构造函数之一：初始化一个大小为cap的空堆
    timer_heap(int cap) throw(exception) :capacity(cap),cur_size(0){
        // 创建堆数组
        array = new heap_timer*[capacity];
        if(array == nullptr){
            throw exception();
        }
        for(int i = 0; i < capacity; ++i){
            array[i] = nullptr;
        }
    }
    // 构造函数之二：用已有的数组来初始化堆
    timer_heap(heap_timer** init_array,int size, int capacity) throw(exception):capacity(capacity),cur_size(size){
        if(capacity < size){
            throw exception();
        }
        // 创建对数组
        array = new heap_timer*[capacity];
        if(array == nullptr){
            throw exception();
        }
        for(int i = 0; i < capacity; ++i){
            array[i] = nullptr;
        }
        if(size != 0){
            // 初始化堆数组
            for(int i = 0; i < size; ++i){
                array[i] = init_array[i];
            }
            for(int i = (cur_size - 1) / 2; i >= 0; i--){
                // 对数组中的第[(cur_size - 1) / 2] ~ 0个元素进行下坠操作
                percolate_down(i);
            }
        }
    }
    // 销毁时间堆
    ~timer_heap(){
        for(int i = 0; i < cur_size; ++i){
            delete array[i];
        }
        delete []array;
    }

public:
    // 添加目标定时器
    void add_timer(heap_timer *timer) throw(exception){
        if(time == nullptr){
            throw exception();
        }
        // 如果当前堆数组容量不够，扩大一倍容量
        if(cur_size >= capacity){
            resize();
        }
        // 新插入了一个元素，当前堆的大小加1，hole是新建空节点的位置
        int hole = cur_size++;
        int parent = 0;
         // 对从空节点到根节点的路径上的所有节点执行上坠操作
         for(; hole > 0; hole = parent){
            // 计算父节点的位置
            parent = (hole - 1) / 2;
            if(array[parent]->expire <= timer->expire){
                break;
            }
            array[hole] = array[parent];
         } 
         array[hole] = timer;
         printf( "add timer %d top %d\n", timer->user_data->sockfd, array[0]->user_data->sockfd );
    }

    // 删除目标定时器
    void del_timer(heap_timer *timer){
        if(timer == nullptr){
            return ;
        }
        /* 仅仅将目标定时器的回调函数设置为空, 即所谓的延迟销毁.
         * 将节省真正删除该定时器造成的开销, 但这样容易使堆数组膨胀 */
        // FIXME: 删除定时器, 为何不重新调整堆?
        timer->cb_func = nullptr;
    }

    // 获得堆顶部的定时器
    heap_timer* top() const{
        if(empty()){
            return nullptr;
        }
        return array[0];
    }

    // 删除堆顶部的定时器
    void pop_timer(){
        if(empty()){
            return ;
        }
        if(array[0] != nullptr){
            delete array[0];
            // 将原来堆顶元素替换为堆数组中最后一个元素
            array[0] = array[--cur_size];
            percolate_down(0);  // 对新的堆顶元素执行下坠操作
        }
    }
    // 心搏函数
    void tick(){
        heap_timer *tmp = array[0];
        time_t cur = time(NULL);    /* 循环处理堆中到期的定时器 */
        while(!empty()){
            if(tmp == nullptr){
                break;
            }
            // 如果堆顶定时器没到期，则退出循环
            if(tmp->expire > cur){
                break;
            }
            // 否则执行堆顶定时器任务
            if(array[0]->cb_func){
                array[0]->cb_func(array[0]->user_data);
            }
            // 将堆顶元素删除，同时生成新的堆顶定时器
            pop_timer();
            printf( "current top is %d\n", array[0]->user_data->sockfd);
            tmp = array[0];
        }
    }
    bool empty() const{
        return cur_size == 0;
    }

private:
    // 最小堆的下坠操作，它确保数组中以第hole个节点为根的子树拥有最小堆性质
    void percolate_down(int hole){
        heap_timer* tmp = array[hole];
        int child = 0;
        for(; (hole * 2 + 1) <= (cur_size - 1); hole = child){
            child = hole * 2 + 1;
            // 找到hole子节点中最小节点child
            if((child < (cur_size - 1)) && (array[child + 1]->expire < array[child]->expire)){
                ++child;
            }
            // 交换hole,child位置对应元素，以满足最小堆特性
            if(array[child]->expire < tmp->expire){
                array[hole] = array[child];
            }else{  /* 否则, 直接跳出继续找子节点的循环. 因为子树中不会存在不满足对特性的节点了 */
                break;
            }
        }
        // 找到hole最终位置，并赋值
        array[hole] = tmp;
    } 

    // 将堆数组容量扩大1倍
    void resize() throw(exception){
        heap_timer** temp = new heap_timer*[2 * capacity];
        for(int i = 0; i < 2 * capacity; ++i){
            temp[i] = nullptr;
        }
        if(temp == nullptr){
            throw exception();
        }
        capacity = 2 * capacity;
        for(int i = 0; i < cur_size; i++){
            temp[i] = array[i];
        }
        delete[] array;
        array = temp;
    }  
private:
    heap_timer** array;
    int capacity;
    int cur_size;

};


#endif