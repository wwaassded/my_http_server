#ifndef LOCKFREE_QUEUE_HPP
#define LOCKFREE_QUEUE_HPP
/*

* 无锁线程安全队列，可以用于替换掉 使用mutex 的 thread_safe_queue

*/

#include <atomic>

namespace what { namespace Log {

//无锁队列 尽量不要存储大型class
template <class __Ty> class LockFreeQueue {
public:
  ~LockFreeQueue(){
    if (content != nullptr) {
      delete[] content;
    }
  } // TODO 如何而进行析构

  LockFreeQueue(int __max_size)
      : max_size(__max_size), __head(0), __tail(0), __tail_update(0) {
    content = new __Ty(max_size);
  }

  auto Pop(__Ty &value)->bool {
    int h;
    do {
      h = __head.load(std::memory_order::memory_order_relaxed);
      if(h == __tail.load(std::memory_order::memory_order_acquire)) {
        return false;
      }
      if(h == this->__tail_update.load(std::memory_order::memory_order_acquire)) {
        return false;
      }
      value = content[h];
    } while(!__head.compare_exchange_strong(h,(h+1)%max_size,std::memory_order::memory_order_release,std::memory_order::memory_order_relaxed));
    return true;
  }

  auto Push(__Ty &value) -> bool {
    int t;
    do {
      t = __tail.load(std::memory_order::memory_order_relaxed);
      if((t+1)%max_size == __head.load(std::memory_order::memory_order_acquire)) {
        return false;
      }
    } while(!__tail.compare_exchange_strong(t,(t+1)%max_size,std::memory_order::memory_order_release,std::memory_order::memory_order_relaxed));
    
    content[t] = value;

    int tail_update = t;
    do {
      tail_update = t;
    } while(__tail_update.compare_exchange_strong(tail_update,(tail_update+1)%max_size,std::memory_order::memory_order_release,std::memory_order::memory_order_relaxed));
    return true;
  }

private:
  std::atomic<int> __head;
  std::atomic<int> __tail;
  std::atomic<int> __tail_update;
  int max_size{0};
  __Ty *content{nullptr};
};

} } // namespace what::Log

#endif