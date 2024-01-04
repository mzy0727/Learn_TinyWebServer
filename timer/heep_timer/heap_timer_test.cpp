#include "heap_timer.h"
#include <unistd.h>

void callback_func(client_data* user_data) {
    // 在这里进行定时器到期时的操作，这里只是简单打印一条消息
    printf("Timer expired! sockfd: %d\n", user_data->sockfd);
}

int main() {
    // 创建时间堆对象
    timer_heap ht(10);

    // 添加一些定时器
    heap_timer* timer1 = new heap_timer(5);  // 5秒后触发
    timer1->cb_func = callback_func;
    client_data data1;
    data1.sockfd = 1;
    data1.timer = timer1;
    timer1->user_data = &data1;
    ht.add_timer(timer1);

    // 添加一些定时器
     heap_timer* timer2 = new heap_timer(10);  // 10秒后触发
    timer2->cb_func = callback_func;
    client_data data2;
    data2.sockfd = 2;
    data2.timer = timer2;
    timer2->user_data = &data2;
    ht.add_timer(timer2);

    // 添加一些定时器
     heap_timer* timer3 = new heap_timer(15);  // 15秒后触发
    timer3->cb_func = callback_func;
    client_data data3;
    data3.sockfd = 3;
    data3.timer = timer3;
    timer3->user_data = &data3;
    ht.add_timer(timer3);

    // 添加一些定时器
     heap_timer* timer4 = new heap_timer(8);  // 15秒后触发
    timer4->cb_func = callback_func;
    client_data data4;
    data4.sockfd = 4;
    data4.timer = timer4;
    timer4->user_data = &data4;
    ht.add_timer(timer4);

    // 模拟时间流逝，每秒调用一次时间轮的 tick 函数
    for (int i = 0; i < 76; ++i) {
        printf( "current i = %d\n",i);
        sleep(1);
        ht.tick();
    }

    return 0;
}
