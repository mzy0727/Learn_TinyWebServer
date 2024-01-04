
# 定时器之时间轮

基于排序链表的定时器存在一个问题：添加定时器的效率偏低。一种简单的时间轮如图所示：

![时间轮](https://glf-1309623969.cos.ap-shanghai.myqcloud.com/img/1625345-20190513163827874-1380641046.png)

在这个时间轮中，实线指针指向轮子上的一个槽（slot）。它以恒定的速度顺时针转动，每转动一步就指向下一个槽（slot）。每次转动称为一个滴答（tick）。一个tick时间间隔为时间轮的`si`（slot interval）。该时间轮共有`N`个槽，因此它转动一周的时间是`N*si`。每个槽指向一条定时器链表，每条链表上的定时器具有相同的特征：它们的定时时间相差`N*si`的整数倍。时间轮正是利用这个关系将定时器散列到不同的链表中。假如现在指针指向槽`cs`，我们要添加一个定时时间为`ti`的定时器，则该定时器将被插入槽`ts`（timer slot）对应的链表中：
$$
ts=(cs+\frac{ti}{si})\%N
$$
这个公式用于计算在时间轮中插入定时器时，定时器应该插入的槽（slot）位置。

- $$
  - cs：当前指针指向的槽的位置。\\
  - \frac{ti}{si}	：定时器的定时时间。\\
  - si：槽间隔，即每个槽代表的时间。\\
  - N：时间轮的槽数。
  $$

具体解释如下：

1. 计算定时器应该插入的槽相对于当前指针指向的槽的偏移量：`ti/si`。这个偏移量表示定时器的定时时间需要经过多少个槽间隔。
2. 将偏移量加上当前指针指向的槽的位置 `cs`，得到插入槽的绝对位置。
3. 使用模运算 `N`，确保插入的槽位置在时间轮的范围内。这是因为时间轮是循环的，超过最大槽数时需要回到起点。

因此，公式的含义是，根据当前指针指向的槽位置和定时器的定时时间，计算出定时器应该插入的槽位置。

基于排序链表的定时器使用唯一的一条链表来管理所有的定时器，所以插入操作的效率随着定时器的数目增多而降低。而时间轮使用了哈希表处理冲突的思想，将定时器散列到不同的链表上。这样每条链表上的定时器数目都将明显少于原来的排序链表上的定时器数目，插入操作的效率基本不受定时器数目的影响。很显然，对于时间轮而言，要**提高精度**，就要使`si`的值足够小; 要**提高执行效率**，则要求`N`值足够大，使定时器尽可能的分布在不同的槽。

## 客户端结构体

这样的结构体通常用于存储与网络编程相关的客户端数据，其中 `tw_timer` 类型的 `timer` 成员可能用于实现定时器功能，用于跟踪特定客户端的超时或其他时间相关事件。

```c++
// 前向声明
class tw_timer;

// 用户数据结构
struct client_data{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];  // 存储数据缓冲区
    tw_timer* timer;
}
```

定义了一个名为 `client_data` 的结构体，其中包含以下成员：

- `address`: 一个 `sockaddr_in` 类型的成员，用于存储网络地址信息。
- `sockfd`: 一个整数类型的成员，用于存储套接字描述符。
- `buf`: 一个字符数组，大小为 `BUFFER_SIZE`，用于存储数据缓冲区。
- `timer`: 一个指向 `tw_timer` 类型对象的指针。这个指针用于关联一个计时器对象到用户数据结构中。

## 基于时间轮的定时器类

构造函数使用成员初始化列表来初始化类的成员变量，而在类中的两个 `public:` 标记之间，有一个 `public` 访问修饰符，表示以下成员（`rotation`、`time_slot`等）都是公有的，可以在类外部访问。

```c++
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

```

- `rotation` 和 `time_slot` 是用于记录定时器在时间轮中的位置的整数成员变量。
- `cb_func` 是一个函数指针，指向一个回调函数，该函数接受一个 `client_data*` 参数，用于在定时器触发时执行特定的操作。
- `user_data` 是一个指向 `client_data` 结构体的指针，用于存储与定时器相关联的用户数据。
- `next` 和 `prev` 是指向下一个和上一个定时器节点的指针，用于在时间轮上组织定时器节点的链表结构。

{% note success %}

- 在C++中，类的构造函数体中的成员初始化列表是在花括号之后定义的，并且这个列表的最后不需要分号。这是因为成员初始化列表的目的是初始化类的成员变量，而不是执行语句块。分号通常用于终结语句，而成员初始化列表不是一个语句块，而是一个在构造函数体之前执行的特殊部分。

{% endnote %}

## 时间轮类

```c++
// 时间轮类：管理定时器
class timer_wheel{
public:

private:
    static const int N = 60;    // 时间轮的槽数
    static const int TI = 1;    // 时间轮的槽间隔
    tw_timer* slots[N];         // 每个槽的头指针，指向定时器链表的头节点
    int cur_slot;               // 当前时间轮指向的槽
};
```

### 构造函数 timer_wheel

这段代码的目的是在创建时间轮对象时，将类的成员变量初始化为合适的初始值。初始化列表用于初始化 `cur_slot`，而循环语句用于将每个槽的头指针初始化为空指针。

```c++
timer_wheel() : cur_slot(0) {
    for (int i = 0; i < N; ++i) {
        slots[i] = nullptr;     // 每个槽点头节点初始化为空
    }
}
```

- `timer_wheel()`: 这是构造函数的定义，表示创建 `timer_wheel` 类的对象时将执行的初始化操作。
- `: cur_slot(0)`: 成员初始化列表的一部分，用于将 `cur_slot` 成员变量初始化为0。这表示时间轮创建时，当前指针指向槽0。
- `for (int i = 0; i < N; ++i)`: 这是一个循环语句，用于遍历时间轮的每个槽。
- `slots[i] = nullptr;`: 在循环中，每次迭代都将 `slots[i]` 初始化为 `nullptr`，表示每个槽的头指针初始为空。这是因为在时间轮创建时，还没有任何定时器加入。

### 析构函数 ~timer_wheel

这段代码的目的是在销毁时间轮对象时，遍历每个槽中的定时器链表，并释放每个定时器节点占用的内存，确保没有内存泄漏。

```c++
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
```

- `~timer_wheel()`: 这是析构函数的定义，表示当 `timer_wheel` 类的对象被销毁时将执行的清理操作。
- `for (int i = 0; i < N; i++)`: 这是一个循环语句，用于遍历时间轮的每个槽。
- `tw_timer* tmp = slots[i];`: 在循环中，首先声明一个指针 `tmp` 并将其初始化为当前槽的头指针 `slots[i]`。
- `while (tmp != nullptr) {`: 这是一个 while 循环，用于遍历当前槽中的定时器链表。
- `slots[i] = tmp->next;`: 将当前槽的头指针指向下一个定时器节点，以准备下一次循环。
- `delete tmp;`: 删除当前定时器节点，释放其在堆上分配的内存。
- `tmp = slots[i];`: 更新 `tmp` 指针，指向当前槽的头指针，以便继续下一次循环。

### 添加新的定时器 add_timer

这个函数的目的是根据给定的超时时间，在时间轮中创建并插入一个定时器，并返回指向该定时器的指针。

```c++
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
        }
        return timer;   // 返回含有时间信息和所在槽位置的定时器
    }

```

- `if (timeout < 0)`: 如果超时时间小于0，表示时间错误，返回空指针。
- `int ticks = (timeout < TI) ? 1 : (timeout / TI);`: 计算定时器在多少个槽间隔时间后触发。
- `int rotation = ticks / N;`: 记录多少圈后定时器生效。
- `int ts = (cur_slot + (ticks % N)) % N;`: 确定插入槽点位置。
- `tw_timer* timer = new tw_timer(rotation, ts);`: 根据位置和圈数创建一个新的 `tw_timer` 对象。
- `if (slots[ts] == nullptr) { ... } else { ... }`: 根据槽中是否已经有定时器节点，采用直接插入或头插法插入新的定时器节点。
- `return timer;`: 返回含有时间信息和所在槽位置的定时器指针。

### 删除目标定时器 del_timer

```c++
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
```

这个函数的目的是从时间轮中删除指定的定时器。

- `if (timer == nullptr) { return; }`: 如果传入的定时器指针为空，直接返回，不执行删除操作。
- `int ts = timer->time_slot;`: 获取定时器所在槽的索引。
- `if (timer == slots[ts]) { ... } else { ... }`: 如果目标定时器是头节点，则特殊处理，否则执行双链表的删除操作。
  - `slots[ts] = slots[ts]->next;`: 更新槽的头节点指针。
  - `if (slots[ts] != nullptr) { slots[ts]->prev = nullptr; }`: 如果新的头节点不为空，更新其前驱指针为 `nullptr`。
  - `delete timer;`: 删除目标定时器。
  - 如果目标定时器不是头节点，执行双链表的删除操作：
    - `timer->prev->next = timer->next;`: 更新前一个节点的后继指针。
    - `if (timer->next != nullptr) { timer->next->prev = timer->prev; }`: 如果目标定时器的后继节点不为空，更新其前驱指针。
    - `delete timer;`: 删除目标定时器。

### 心跳函数 tick

```c++
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
```

这个函数是时间轮的心跳函数，用于处理定时器的到期事件。函数逻辑如下：

- `tw_timer* tmp = slots[cur_slot];`: 获取当前槽头节点指针。
- `printf("current slot is %d\n", cur_slot);`: 打印当前槽的索引。
- `while (tmp != nullptr) { ... }`: 遍历当前槽所指向的链表。
  - `if (tmp->rotation > 0) { ... } else { ... }`: 如果定时器的剩余轮数大于0，减少轮数并移动到下一个定时器。
  - 如果定时器剩余轮数为0，表示定时器到期，执行以下操作：
    - `tmp->cb_func(tmp->user_data);`: 调用定时器的回调函数。
    - 判断是否是头节点：
      - 如果是头节点：
        - `printf("delete header in cur_slot\n");`: 打印删除头节点的消息。
        - `slots[cur_slot] = tmp->next;`: 更新头节点指针。
        - `delete tmp;`: 删除当前定时器。
        - 如果新的头节点不为空，更新其前驱指针。
        - `tmp = slots[cur_slot];`: 更新 tmp 指针。
      - 如果不是头节点：
        - `tmp->prev->next = tmp->next;`: 更新前一个节点的后继指针。
        - 如果定时器的后继节点不为空，更新其前驱指针。
        - `tw_timer* tmp2 = tmp->next;`: 保存下一个定时器节点的指针。
        - `delete tmp;`: 删除当前定时器。
        - `tmp = tmp2;`: 更新 `tmp` 指针。
- `cur_slot = ++cur_slot % N;`: 轮子往后走一个槽，更新当前槽的索引。

{% note success %}

`tmp = slots[cur_slot];` 这行代码的目的是将 `tmp` 指针重新指向当前槽的头节点，以便在下一轮循环中继续遍历当前槽所指向的定时器链表。

在心跳函数中，`tmp` 指针用于遍历当前槽的定时器链表，处理每个定时器的情况。当一个定时器到期并被处理后，可能会影响当前槽的链表结构（例如删除头节点），所以在处理完一个定时器后，需要重新将 `tmp` 指针指向当前槽的新的头节点，以确保下一次循环可以正确遍历整个链表。

{% endnote %}

## 单元测试

```c++
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
```

```txt
add timer, rotation is 0, ts is 5, cur_slot is 0
add timer, rotation is 0, ts is 10, cur_slot is 0
add timer, rotation is 0, ts is 15, cur_slot is 0
add timer, rotation is 1, ts is 5, cur_slot is 0
add timer, rotation is 1, ts is 15, cur_slot is 0
current slot is 0
current slot is 1
current slot is 2
current slot is 3
current slot is 4
current slot is 5
tick the timer once
tick the timer once
Timer expired! sockfd: 1
delete ordinary node in cur_slot
current slot is 6
current slot is 7
current slot is 8
current slot is 9
current slot is 10
tick the timer once
Timer expired! sockfd: 2
delete header in cur_slot
current slot is 11
current slot is 12
current slot is 13
current slot is 14
current slot is 15
tick the timer once
tick the timer once
Timer expired! sockfd: 3
delete ordinary node in cur_slot
current slot is 16
.
.
.
current slot is 59
current slot is 0
current slot is 1
current slot is 2
current slot is 3
current slot is 4
current slot is 5
tick the timer once
Timer expired! sockfd: 4
delete header in cur_slot
current slot is 6
current slot is 7
current slot is 8
current slot is 9
current slot is 10
current slot is 11
current slot is 12
current slot is 13
current slot is 14
current slot is 15
tick the timer once
Timer expired! sockfd: 5
delete header in cur_slot
```

