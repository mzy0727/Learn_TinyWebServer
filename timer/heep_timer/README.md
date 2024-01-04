# 定时器之时间堆

除了以固定频率调用心搏函数 `tick()` 的定时方案，另一种思路是：将所有定时器中超时时间最小的一个定时器的超时值作为心搏间隔。这样，一旦心搏函数 `tick()` 被调用，超时时间最小的定时器必然到期， 我们就可以在 `tick()` 函数中处理该定时器。然后，再次从剩余的定时器中找出超时时间最小的一个，并将这段最小时间设置为下一次心搏间隔。如此反复，就实现了较为精确的定时。

## 客户端结构体

这段程序的目的是为了在一个系统中管理客户端的数据，使用了堆定时器来进行与客户端相关的定时操作。

```c++
class heap_timer;
// 用户数据结构
struct client_data{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];  // 存储数据缓冲区
    heap_timer* timer;
};
```

这是一个名为 `client_data` 的结构体，用于存储与客户端相关的数据。结构体的成员包括：

- `address`：一个 `sockaddr_in` 类型的成员，用于存储客户端的地址信息。
- `sockfd`：一个整数，用于存储与客户端关联的套接字描述符。
- `buf`：一个字符数组，用于存储数据缓冲区，大小为 `BUFFER_SIZE`。
- `timer`：一个指向 `heap_timer` 类的指针，用于关联客户端数据与堆定时器。

## 基于堆的定时器类

定义了一个名为 `heap_timer` 的类，用于表示堆定时器。这个类包含了一个过期时间 `expire`，一个回调函数指针 `cb_func`，以及一个指向 `client_data` 结构体的指针 `user_data`。

```c++
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
```

这个类的实例在构造时计算了过期时间，过期时间的计算基于当前时间加上传入的延时参数。这样，你可以通过创建 `heap_timer` 的实例来表示一个堆定时器，并设置回调函数和用户数据。

注意，这个类中使用了 `time_t` 类型，它是一个表示时间的数据类型。`time(NULL)` 返回当前时间的秒数，因此 `expire` 表示的是定时器的过期时刻。在实际使用中，你可以创建一个 `heap_timer` 实例，设置回调函数和用户数据，然后使用这个实例来管理定时器。

## 时间堆类

**`timer_heap` 类**：

- `timer_heap` 类表示一个基于最小堆的时间堆定时器，包含以下成员变量和方法：
  - `array`：堆数组，存储堆定时器。
  - `capacity`：堆数组的容量。
  - `cur_size`：当前堆数组中包含的元素个数。
- 构造函数：
  - 构造函数1：初始化一个大小为 `cap` 的空堆。
  - 构造函数2：用已有的数组 `init_array` 来初始化堆，同时指定堆的大小和容量。
- 析构函数：释放堆数组中的定时器内存。
- `add_timer` 方法：向堆中添加目标定时器，如果堆数组容量不够，则扩大一倍容量，并执行上虑操作。
- `del_timer` 方法：将目标定时器的回调函数设置为空，实现延迟销毁。
- `top` 方法：获取堆顶部的定时器。
- `pop_timer` 方法：删除堆顶部的定时器，同时执行下虑操作。
- `tick` 方法：心跳函数，处理堆数组中到期的定时器，执行定时器的回调函数。
- `empty` 方法：判断堆数组是否为空。
- `percolate_down` 方法：最小堆的下虑操作，确保堆数组中以指定节点作为根的子树拥有最小堆的性质。
- `resize` 方法：将堆数组容量扩大一倍。

### 构造函数 timer_heap(int cap)

目的是在堆上动态分配一个数组，用于存储堆定时器的指针，并将数组中的每个元素初始化为 `nullptr`，表示初始时堆为空。这是 `time_heap` 类中的一个构造函数，用于创建一个指定容量的空堆。

```c++
// 构造函数之一：初始化一个大小为cap的空堆
timer_heap(int cap) throw(exception) : capacity(cap), cur_size(0) {
    // 创建堆数组
    array = new heap_timer*[capacity];
    if(array == nullptr){
        throw exception();
    }
    for(int i = 0; i < capacity; ++i){
        array[i] = nullptr;
    }
}
```

- `timer_heap(int cap) throw(exception)`：构造函数的声明，接受一个整数参数 `cap`，表示初始化堆的容量。
- `: capacity(cap), cur_size(0)`：成员初始化列表，用于对类成员进行初始化。`capacity` 被初始化为 `cap`，表示堆的容量；`cur_size` 被初始化为 0，表示当前堆中元素的个数。
- `array = new heap_timer*[capacity]`：在堆上分配一个 `heap_timer*` 类型的数组，用于存储堆定时器的指针。`capacity` 决定了数组的大小。
- `if(array == nullptr)`：检查数组是否成功分配。如果分配失败，即 `new` 操作返回 `nullptr`，则抛出异常。
- `for(int i = 0; i < capacity; ++i)`：遍历数组，将每个元素初始化为 `nullptr`，确保数组中的指针都初始化为 `nullptr`，表示初始时堆中没有定时器。

### 最小堆的下沉操作 percolate_down

这段代码实现了保持最小堆性质的下沉操作，确保以指定节点为根的子树拥有最小堆性质。在堆的插入和删除操作中，通过不断执行下沉操作，可以维护整个堆的最小堆性质。

```c++
// 最小堆的下沉操作，它确保数组中以第hole个节点为根的子树拥有最小堆性质
void percolate_down(int hole) {
    heap_timer* tmp = array[hole];
    int child = 0;

    for (; (hole * 2 + 1) <= (cur_size - 1); hole = child) {
        child = hole * 2 + 1;

        // 找到hole子节点中最小的节点child
        if ((child < (cur_size - 1)) && (array[child + 1]->expire < array[child]->expire)) {
            ++child;
        }

        // 如果子节点的过期时间小于当前节点的过期时间，则交换它们的位置
        if (array[child]->expire < tmp->expire) {
            array[hole] = array[child];
        } else {
            // 否则，直接跳出循环，因为子树中不会存在不满足最小堆性质的节点了
            break;
        }
    }

    // 找到hole最终位置，并赋值
    array[hole] = tmp;
}
```

- `heap_timer* tmp = array[hole];`：保存当前节点的指针，准备进行下沉操作。
- `int child = 0;`：初始化子节点的索引。
- `for (; (hole * 2 + 1) <= (cur_size - 1); hole = child)`：循环进行下沉操作，直到当前节点没有子节点或者满足最小堆性质。
- `child = hole * 2 + 1;`：计算左子节点的索引。
- `if ((child < (cur_size - 1)) && (array[child + 1]->expire < array[child]->expire))`：如果右子节点存在且比左子节点的过期时间小，就选择右子节点作为最小的子节点。
- `if (array[child]->expire < tmp->expire)`：如果最小的子节点的过期时间小于当前节点的过期时间，则交换它们的位置。
- `else { break; }`：如果当前节点的过期时间已经小于等于最小的子节点，说明已经满足最小堆性质，直接跳出循环。
- `array[hole] = tmp;`：将当前节点移动到最终位置，保持最小堆性质。

### 构造函数 timer_heap(heap_timer** init_array, int size, int capacity)

这段代码的目的是在堆上动态分配一个数组，用于存储堆定时器的指针，并根据传入的初始化数组进行初始化。初始化完成后，通过执行下滤操作，确保整个数组满足最小堆性质。这是 `time_heap` 类中的另一个构造函数，用于创建一个包含初始元素的堆。

```c++
// 构造函数之二：用已有的数组来初始化堆
timer_heap(heap_timer** init_array, int size, int capacity) throw(exception) : capacity(capacity), cur_size(size) {
    if (capacity < size) {
        throw exception();
    }
    // 创建堆数组
    array = new heap_timer*[capacity];
    if (array == nullptr) {
        throw exception();
    }
    for (int i = 0; i < capacity; ++i) {
        array[i] = nullptr;
    }
    if (size != 0) {
        // 初始化堆数组
        for (int i = 0; i < size; ++i) {
            array[i] = init_array[i];
        }
        for (int i = (cur_size - 1) / 2; i >= 0; i--) {
            // 对数组中的第[(cur_size - 1) / 2] ~ 0个元素进行下坠操作
            percolate_down(i);
        }
    }
}
```

- `timer_heap(heap_timer** init_array, int size, int capacity) throw(exception)`：构造函数的声明，接受一个指向 `heap_timer*` 数组的指针 `init_array`，以及数组的大小 `size` 和堆的容量 `capacity`。
- 在这里，`: throw(exception)` 意味着该构造函数声明或定义时声明了一个异常规范（exception specification）。这个异常规范表示该构造函数可能抛出任何异常，但是如果它抛出异常，它将是 `std::exception` 类型的异常。**在现代 C++ 中，异常规范已经被认为是过时的，通常不建议使用。**
- `: capacity(capacity), cur_size(size)`：成员初始化列表，用于对类成员进行初始化。`capacity` 被初始化为传入的 `capacity`，表示堆的容量；`cur_size` 被初始化为传入的 `size`，表示当前堆中元素的个数。
- `if (capacity < size)`：检查传入的容量是否小于数组的大小，如果是则抛出异常。
- `array = new heap_timer*[capacity];`：在堆上分配一个 `heap_timer*` 类型的数组，用于存储堆定时器的指针。`capacity` 决定了数组的大小。
- `if (array == nullptr)`：检查数组是否成功分配。如果分配失败，即 `new` 操作返回 `nullptr`，则抛出异常。
- `for (int i = 0; i < capacity; ++i)`：遍历数组，将每个元素初始化为 `nullptr`，确保数组中的指针都初始化为 `nullptr`，表示初始时堆为空。
- `if (size != 0)`：如果传入的数组大小不为零，说明有初始化的元素，进行以下操作：
  - 遍历初始化数组，将其中的元素拷贝到堆数组中。
  - 对数组中的前半部分元素（即从最后一个非叶子节点开始）执行下滤操作，确保整个数组满足最小堆性质。

### 析构函数 ~timer_heap

这段代码的目的是确保在销毁 `timer_heap` 对象时，释放它所管理的所有 `heap_timer` 对象的内存，以及释放堆数组本身的内存。这是一种良好的实践，以防止内存泄漏。

```c++
// 销毁时间堆
~timer_heap() {
    for (int i = 0; i < cur_size; ++i) {
        delete array[i];
    }
    delete[] array;
}
```

- `~timer_heap()`: 析构函数的声明，表示当对象被销毁时会执行这段代码。
- `for (int i = 0; i < cur_size; ++i)`: 循环遍历堆数组中的每个元素。
- `delete array[i]`: 删除堆数组中每个元素指向的 `heap_timer` 对象。这个操作会调用相应对象的析构函数。
- `delete[] array`: 删除堆数组本身，释放数组占用的内存。

### 堆数组容量 resize

这段代码的目的是在堆数组容量不足时，动态地将其扩大一倍。这是一种动态管理内存的策略，以适应动态变化的堆大小需求。

```c++
// 将堆数组容量扩大1倍
void resize() throw(exception) {
    heap_timer** temp = new heap_timer*[2 * capacity];
    for (int i = 0; i < 2 * capacity; ++i) {
        temp[i] = nullptr;
    }
    if (temp == nullptr) {
        throw exception();
    }
    capacity = 2 * capacity;
    for (int i = 0; i < cur_size; i++) {
        temp[i] = array[i];
    }
    delete[] array;
    array = temp;
}
```

- `void resize() throw(exception)`: `resize` 是一个成员函数，用于将堆数组容量扩大一倍。`throw(exception)` 表示该函数可能抛出 `std::exception` 类型的异常。
- `heap_timer** temp = new heap_timer*[2 * capacity];`: 在堆上动态分配一个新的数组 `temp`，其大小是原来数组容量的两倍。
- `for (int i = 0; i < 2 * capacity; ++i)`: 初始化新数组中的每个元素为 `nullptr`。
- `if (temp == nullptr)`: 检查新数组是否成功分配。如果分配失败，即 `new` 操作返回 `nullptr`，则抛出异常。
- `capacity = 2 * capacity;`: 更新堆数组的容量。
- `for (int i = 0; i < cur_size; i++)`: 将原数组中的元素拷贝到新数组中。
- `delete[] array;`: 删除原来的堆数组，释放原数组占用的内存。
- `array = temp;`: 将类成员 `array` 指向新的数组，完成扩容。

### 添加目标定时器 add_timer

这段代码的目的是将一个新的定时器插入到时间堆中，并确保时间堆的最小堆性质仍然得以保持。

```c++
// 添加目标定时器
void add_timer(heap_timer* timer) throw(exception) {
    if (timer == nullptr) {
        throw exception();
    }
    // 如果当前堆数组容量不够，扩大一倍容量
    if (cur_size >= capacity) {
        resize();
    }
    // 新插入了一个元素，当前堆的大小加1，hole是新建空节点的位置
    int hole = cur_size++;
    int parent = 0;
    // 对从空节点到根节点的路径上的所有节点执行上浮操作
    for (; hole > 0; hole = parent) {
        // 计算父节点的位置
        parent = (hole - 1) / 2;
        if (array[parent]->expire <= timer->expire) {
            break;
        }
        array[hole] = array[parent];
    }
    array[hole] = timer;
}
```

- `void add_timer(heap_timer* timer) throw(exception)`: `add_timer` 是一个成员函数，用于向时间堆中添加目标定时器。`throw(exception)` 表示该函数可能抛出 `std::exception` 类型的异常。
- `if (timer == nullptr)`: 检查传入的定时器指针是否为 `nullptr`。如果是，抛出异常。
- `if (cur_size >= capacity)`: 检查当前堆数组容量是否不足。如果不足，调用 `resize()` 函数进行扩容。
- `int hole = cur_size++;`: `hole` 表示新插入的元素的位置，同时递增 `cur_size` 表示当前堆的大小。
- `int parent = 0;`: 初始化父节点的位置为根节点。
- `for (; hole > 0; hole = parent)`: 循环执行上浮操作，直到新插入的元素找到合适的位置。
- `parent = (hole - 1) / 2;`: 计算当前节点的父节点的位置。
- `if (array[parent]->expire <= timer->expire)`: 检查父节点的定时器是否小于等于新插入的定时器。如果是，说明已经找到了合适的位置，跳出循环。
- `array[hole] = array[parent];`: 将父节点的定时器下移到当前节点的位置。
- `array[hole] = timer;`: 将新插入的定时器放置到当前节点的位置。

### 删除目标定时器 del_timer

这段代码的目的是通过将目标定时器的回调函数设置为空，实现对定时器的“延迟销毁”。

```c++
// 删除目标定时器
void del_timer(heap_timer* timer) {
    if (timer == nullptr) {
        return;
    }
    /* 仅仅将目标定时器的回调函数设置为空, 即所谓的延迟销毁.
     * 将节省真正删除该定时器造成的开销, 但这样容易使堆数组膨胀 */
    // FIXME: 删除定时器, 为何不重新调整堆?
    timer->cb_func = nullptr;
}
```

- `void del_timer(heap_timer* timer)`: `del_timer` 是一个成员函数，用于删除目标定时器。
- `if (timer == nullptr)`: 检查传入的定时器指针是否为 `nullptr`。如果是，直接返回，不执行删除操作。
- `timer->cb_func = nullptr;`: 将目标定时器的回调函数指针设置为空，即延迟销毁。这种操作确实可以避免直接删除定时器对象造成的开销，但是作者在注释中提到这样容易使堆数组膨胀。注释中还提到了一个 FIXME（修复），暗示这里的处理方式可能不是最理想的。在实际使用中，如果要删除定时器并避免膨胀，可能需要对时间堆的底层数据结构进行更多的调整。

### 优化 删除目标定时器 del_timer

这个修改中，添加了 `percolate_up` 和 `percolate_down` 函数，用于分别执行上浮和下沉操作，以保持最小堆的性质。需要根据具体实现，调整这两个函数的实现以适应代码。这样，删除定时器时就会真正地从堆中去除节点。

```c++
// 删除目标定时器
void del_timer(heap_timer* timer) {
    if (timer == nullptr) {
        return;
    }

    // 找到目标定时器在堆数组中的位置
    for (int i = 0; i < cur_size; ++i) {
        if (array[i] == timer) {
            // 将目标定时器的位置上的元素替换为堆数组中最后一个元素
            array[i] = array[cur_size - 1];
            cur_size--;

            // 调整堆，保持最小堆的性质
            if (i != 0 && array[(i - 1) / 2]->expire > array[i]->expire) {
                percolate_up(i);
            } else {
                percolate_down(i);
            }

            break;
        }
    }
}

```

### 获得堆顶部的定时器 top

这段代码的目的是提供一种获取堆顶部定时器的方式，同时考虑了堆为空的情况，以防止访问空数组。

```c++
// 获得堆顶部的定时器
heap_timer* top() const {
    if (empty()) {
        return nullptr;
    }
    return array[0];
}
```

- `heap_timer* top() const`: `top` 是一个成员函数，用于获取堆顶部的定时器。`const` 表示该函数不会修改成员变量。
- `if (empty())`: 调用 `empty()` 函数来检查堆是否为空。
- `return array[0];`: 如果堆不为空，则返回堆数组中的第一个元素，即堆顶部的定时器。
- `return nullptr;`: 如果堆为空，则返回 `nullptr` 表示没有定时器。

### 删除堆顶部的定时器 pop_timer

这段代码的目的是删除堆顶部的定时器，并通过将堆顶元素替换为堆数组中最后一个元素，并执行下坠操作，以保持最小堆的性质。

```c++
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
```

- `void pop_timer()`: `pop_timer` 是一个成员函数，用于删除堆顶部的定时器。
- `if (empty())`: 调用 `empty()` 函数来检查堆是否为空。
- `if (array[0] != nullptr)`: 检查堆顶部的定时器是否为空。
- `delete array[0];`: 删除堆顶部的定时器。
- `array[0] = array[--cur_size];`: 将原来堆顶元素替换为堆数组中最后一个元素，并减小堆的大小。
- `percolate_down(0);`: 对新的堆顶元素执行下坠操作，以保持最小堆的性质。

### 心搏函数 tick

这段代码的目的是在循环中处理堆中到期的定时器，执行其任务，并将其删除，直到堆中没有到期的定时器或者堆为空。

```c++
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
        tmp = array[0];
    }
}
```

- `void tick()`: `tick` 是一个成员函数，用于处理到期的定时器。
- `heap_timer* tmp = array[0];`: 获取堆顶部的定时器。
- `time_t cur = time(NULL);`: 获取当前时间。
- `while (!empty())`: 进入循环，处理堆中到期的定时器。
- `if (tmp == nullptr)`: 检查堆顶定时器是否为空，如果是，则退出循环。
- `if (tmp->expire > cur)`: 如果堆顶定时器的到期时间大于当前时间，说明后面的定时器都还没有到期，退出循环。
- `if (array[0]->cb_func)`: 如果堆顶定时器的回调函数不为空，执行回调函数，即执行定时器任务。
- `pop_timer()`: 将堆顶元素删除，同时生成新的堆顶定时器，继续循环。

## 单元测试

```c++
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

```

```txt
add timer 1 top 1
add timer 2 top 1
add timer 3 top 1
add timer 4 top 1
current i = 0
current i = 1
current i = 2
current i = 3
current i = 4
Timer expired! sockfd: 1
current top is 4
current i = 5
current i = 6
current i = 7
Timer expired! sockfd: 4
current top is 2
current i = 8
current i = 9
Timer expired! sockfd: 2
current top is 3
current i = 10
current i = 11
current i = 12
current i = 13
current i = 14
Timer expired! sockfd: 3
current top is 3
current i = 15
current i = 16
current i = 17
current i = 18
current i = 19
.
.
.
current i = 71
current i = 72
current i = 73
current i = 74
current i = 75
```

