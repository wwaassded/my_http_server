#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <mutex>

namespace what::Log {

static const int internal_wait_ms = 100;  // TODO 可能需要一个更好的数值

template <class __Ty>
class Block_queue {
 public:
  Block_queue(size_t __max_size) : max_size(__max_size), size(0) { content = malloc(sizeof(__Ty) * max_size); }

  ~Block_queue() {
    if (content != nullptr) {
      delete[] content;
    }
  }

  auto Size() -> size_t {
    size_t tmp = 0;
    std::unique_lock<std::mutex> guard(lock);
    tmp = size;
    return tmp;
  }

  auto Max_size() -> size_t {
    size_t tmp = 0;
    std::unique_lock<std::mutex> guard(lock);
    tmp = max_size;
    return tmp;
  }

  auto Is_empty() -> bool {
    std::unique_lock<std::mutex> guard(lock);
    if (size == 0) return true;
    return false;
  }

  auto Is_full() -> bool {
    std::unique_lock<std::mutex> guard(lock);
    if (size == max_size) return true;
    return false;
  }

  // 非阻塞的
  auto Back(__Ty &value) -> bool {
    std::unique_lock<std::mutex> guard(lock);
    if (size == 0) return false;
    value = content[tail];
    return true;
  }
  auto Front(__Ty &value) -> bool {
    std::unique_lock<std::mutex> guard(lock);
    if (size == 0) return false;
    value = content[head];
    return true;
  }

  // push非阻塞的
  bool Push(const __Ty &value)->bool {
    std::unique_lock<std::mutex> guard(lock);
    if (size >= max_size) {
      //? pop_cv.notify_one();
      return false;
    }
    content[head] = value;
    head = (head + 1) % max_size;
    ++size;
    pop_cv.notify_one();
    return true;
  }
  // pop阻塞
  void Pop(__Ty &value) {
    std::unique_lock<std::mutex> guard(lock);
    pop_cv.wait_for(guard, [this] { return size != 0; });
    value = content[tail];
    tail = (tail + 1) % max_size;
    --size;
  }
  // 等待指定时间的函数
  auto Pop_time_wait(__Ty &value) -> bool {
    std::unique_lock<std::mutex> guard(lock);
    //? 🤔 虚假唤醒 传递一个 predict 无需使用在额外使用while循环保证
    //? 标准库已经为我们设计好啦 😁
    if (!pop_cv.wait_until(guard,
                           std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(internal_wait_ms),
                           [this]() { return size != 0; })) {
      return false;
    }
    value = content[tail];
    tail = (tail + 1) % max_size;
    --size;
    return true;
  }

  void clear() {
    std::unique_lock<std::mutex> guard(lock);
    head = 0;
    tail = 0;
    size = 0;
  }

 private:
  __Ty *content;
  size_t max_size;
  size_t size;
  size_t head{0};  //输入端
  size_t tail{0};  //输出端
  std::mutex lock;
  std::condition_variable pop_cv;  // pop阻塞使用的条件变量
};

}  // namespace what::Log

#endif