#ifndef MY_TIMER_HPP
#define MY_TIMER_HPP
#include <time.h>
#include <arpa/inet.h>

//*时钟的设计 应该是一个有序的链表 根据超时时间排序 实现链表基础的添加节点 删除节点 遍历查找过期的节点
//*实现上考虑使用单向链表 因为节点相对简单 如果假如两个8字节的指针过于浪费空间了

struct __timer_node;

struct __user_data // 链接资源
{
    int __fd;
    sockaddr_in __client_address;
    __timer_node *__timer;
};

struct __timer_node
{
    __timer_node *__next{nullptr};
    time_t __expired_time;
    void (*__expired_call_back_function)(void *);
    __user_data *__data;
};

//* 应该不涉及到并发访问问题
class Timer
{
public:
    Timer() : __timer_chain(nullptr) {}
    Timer(__timer_node *start_chain) : __timer_chain(start_chain) {} //? 避免构造函数中 new抛出异常 引起不必要的麻烦

    ~Timer();

    void Add_Timer(time_t); // 几乎没什么实用的价值

    void Add_Timer_Node(__timer_node *); // 使用更多的添加函数

    bool Refresh_Timer(__timer_node *, time_t); // 刷新timer中的节点

    void Tick(); // 处理所有超时的节点

private:
    bool remove_from_chain(__timer_node *); // 并不会free node

    __timer_node *__timer_chain{nullptr};
};

#endif