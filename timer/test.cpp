#include <gtest/gtest.h>
#include "/media/mzy/learn_TinyWebServer/timer/lst_timer.h"

// 测试定时器链表的基本功能
TEST(TimerListTest, BasicFunctionality) {
    // 创建定时器链表
    sort_timer_lst timer_list;

    // 创建两个客户端数据
    client_data client1;
    client_data client2;

    // 创建两个定时器
    util_timer timer1;
    util_timer timer2;

    // 设置定时器的回调函数
    timer1.cb_func = cb_func;
    timer2.cb_func = cb_func;

    // 设置定时器的超时时间
    timer1.expire = time(nullptr) + 5;
    timer2.expire = time(nullptr) + 10;

    // 设置定时器的用户数据
    timer1.user_data = &client1;
    timer2.user_data = &client2;

    // 添加定时器到链表
    timer_list.add_timer(&timer1);
    timer_list.add_timer(&timer2);

    // 断言链表头尾指针正确
   // ASSERT_EQ(timer_list.get_head(), &timer1);
   // ASSERT_EQ(timer_list.get_tail(), &timer2);

    // 调整定时器，使第一个定时器提前触发
    timer1.expire = time(nullptr) - 1;
    timer_list.adjust_timer(&timer1);

    // 断言链表头尾指针正确
  //  ASSERT_EQ(timer_list.get_head(), &timer2);
 //   ASSERT_EQ(timer_list.get_tail(), &timer1);

    // 执行定时任务处理函数
    timer_list.tick();

    // 断言链表为空
  //  ASSERT_EQ(timer_list.get_head(), nullptr);
 //   ASSERT_EQ(timer_list.get_tail(), nullptr);
}

int main(int argc, char **argv) {
    // 初始化 Google Test 框架
    ::testing::InitGoogleTest(&argc, argv);
    // 执行所有测试用例
    return RUN_ALL_TESTS();
}
