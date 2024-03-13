#include <cassert>
#include <vector>
#include <timer.hpp>
#include <log_what.hpp>

Timer::~Timer()
{
    if (__timer_chain != nullptr)
    {
        auto ptr = __timer_chain->__next;
        while (ptr != nullptr)
        {
            delete __timer_chain;
            __timer_chain = ptr;
            ptr = ptr->__next;
            // TODO: 可能会需要调用超时的回调函数
        }
        delete __timer_chain;
        __timer_chain = nullptr;
    }
}

void Timer::Add_Timer(time_t expired_time)
{
    __timer_node *new_node = new __timer_node();
    new_node->__expired_time = expired_time; // TODO: timer的初始化问题
    if (__timer_chain == nullptr)
        __timer_chain = new __timer_node();
    if (__timer_chain->__next == nullptr)
    {
        __timer_chain->__next = new_node;
        return;
    }
    auto ptr = __timer_chain;
    while (ptr->__next != nullptr)
    {
        if (ptr->__expired_time >= expired_time)
        {
            new_node->__next = ptr->__next;
            ptr->__next = new_node;
            break;
        }
    }
    if (ptr->__next == nullptr)
    {
        ptr->__next = new_node;
        new_node->__next = nullptr; // just make sure
    }
}

void Timer::Add_Timer_Node(__timer_node *node)
{
    assert(node != nullptr);
    if (__timer_chain == nullptr)
        __timer_chain = new __timer_node();
    if (__timer_chain->__next == nullptr)
    {
        __timer_chain->__next = node;
        return;
    }
    auto ptr = __timer_chain;
    while (ptr->__next != nullptr)
    {
        if (ptr->__expired_time >= node->__expired_time)
        {
            node->__next = ptr->__next;
            ptr->__next = node;
            break;
        }
    }
    if (ptr->__next == nullptr)
    {
        ptr->__next = node;
        node->__next = nullptr; // just make sure
    }
}

bool Timer::Remove_from_chain(__timer_node *node)
{
    if (node == nullptr || __timer_chain == nullptr)
        return false;
    auto expired_time = node->__expired_time;
    auto ptr = __timer_chain->__next;
    while (ptr->__next != nullptr && ptr->__next->__expired_time <= expired_time)
    {
        if (ptr->__next == node)
        {
            ptr->__next = ptr->__next->__next;
            return true;
        }
    }
    return false;
}

bool Timer::Refresh_Timer(__timer_node *node, time_t new_expired_time)
{
    if (node == nullptr || __timer_chain == nullptr || node->__expired_time == new_expired_time)
        return false;
    assert(Remove_from_chain(node));
    node->__expired_time = new_expired_time;
    Add_Timer_Node(node);
    return true;
}

void Timer::Tick()
{
    if (__timer_chain == nullptr)
    {
        LOG(ERROR, "定时器没有得到及时的初始化");
        assert(false);
    }
    time_t now_since_epoch = time(NULL); //* 获取当前的时间精度只有秒 但是应该足够了
    std::vector<__timer_node *> __expired_queue;
    auto ptr = __timer_chain;
    while (ptr != nullptr && ptr->__next != nullptr && ptr->__next->__expired_time <= now_since_epoch)
    {
        __expired_queue.push_back(ptr->__next);
        ptr->__next = ptr->__next->__next;
        __expired_queue.back()->__next = nullptr;
    }
    for (auto &item : __expired_queue)
    {
        item->__expired_call_back_function(item->__data);
        delete item;
        item = nullptr;
    }
}