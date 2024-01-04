#include "timer_wheel.h"
#include <unistd.h>

void callback_func(client_data* user_data) {
    // 在这里进行定时器到期时的操作，这里只是简单打印一条消息
    printf("Timer expired! sockfd: %d\n", user_data->sockfd);
}

int main() {
    // 创建时间轮对象
    timer_wheel tw;

    // 添加一些定时器
    tw_timer* timer1 = tw.add_timer(5);  // 5秒后触发
    timer1->cb_func = callback_func;
    client_data data1;
    data1.sockfd = 1;
    data1.timer = timer1;
    timer1->user_data = &data1;

    tw_timer* timer2 = tw.add_timer(10);  // 10秒后触发
    timer2->cb_func = callback_func;
    client_data data2;
    data2.sockfd = 2;
    data2.timer = timer2;
    timer2->user_data = &data2;

    tw_timer* timer3 = tw.add_timer(15);  // 15秒后触发
    timer3->cb_func = callback_func;
    client_data data3;
    data3.sockfd = 3;
    data3.timer = timer3;
    timer3->user_data = &data3;

    tw_timer* timer4 = tw.add_timer(65);  // 15秒后触发
    timer4->cb_func = callback_func;
    client_data data4;
    data4.sockfd = 4;
    data4.timer = timer4;
    timer4->user_data = &data4;

    tw_timer* timer5 = tw.add_timer(75);  // 15秒后触发
    timer5->cb_func = callback_func;
    client_data data5;
    data5.sockfd = 5;
    data5.timer = timer5;
    timer5->user_data = &data5;

    // 模拟时间流逝，每秒调用一次时间轮的 tick 函数
    for (int i = 0; i < 76; ++i) {
        sleep(1);
        tw.tick();
    }

    return 0;
}
