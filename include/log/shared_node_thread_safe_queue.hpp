#ifndef SHARED_NODE_THREAD_SAFE_QUEUE_HPP
#define SHARED_NODE_THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

namespace what::Log
{
    template <class __T>
    class SharedNodeThreadSafeQueue
    {
    private:
        std::queue<std::shared_ptr<__T>> queue;
        mutable std::mutex locker;
        std::condition_variable cv;

    public:
        auto is_empty() -> bool
        {
            std::unique_lock<std::mutex> tmp_lock(locker);
            return queue.empty();
        }

        auto wait_and_pop() -> std::shared_ptr<__T>
        {
            std::unique_lock<std::mutex> tmp_lock(locker);
            cv.wait(tmp_lock, [this]
                    { return !queue.empty(); });
            auto it = queue.front();
            queue.pop();
            return it;
        }

        auto wait_and_pop(__T &value) -> void
        {
            std::unique_lock<std::mutex> tmp_lock(locker);
            cv.wait(tmp_lock, [this]
                    { return !queue.empty(); });
            value = std::move(*queue.front());
            queue.pop();
        }

        auto try_pop() -> std::shared_ptr<__T>
        {
            std::unique_lock<std::mutex> tmp_lock(locker);
            if (queue.empty())
                return std::shared_ptr<__T>();
            auto it = queue.front();
            queue.pop();
            return it;
        }

        auto try_pop(__T &value) -> bool
        {
            std::unique_lock<std::mutex> tmp_lock(locker);
            if (queue.empty())
                return false;
            value = std::move(*queue.front());
            queue.pop();
            return true;
        }

        auto push(__T value) -> void
        {
            //? 先构造智能指针 在对queue进行操作 以免对队列中的数据造成污染
            auto it = std::make_shared<__T>(std::move(value));
            std::unique_lock<std::mutex> tmp_lock(locker);
            queue.push(it);
            cv.notify_one();
        }
    };
}

#endif