#ifndef THREAD_SAFE_LIST_HPP
#define THREAD_SAFE_LIST_HPP

template <class Ty>
class Thread_Safe_List
{
public:
    Thread_Safe_List(int cap) : max_len(cap)
    {
    }
    using value_type = Ty;
    using ptr_type = Ty *;
    using constptr_type = const Ty *;

private:
    int max_len;
};

#endif