#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <http_connection.hpp>
#include <atomic>
#include <thread_pool_continent_base.hpp>
#include <log_what.hpp>

static const unsigned THREAD_MIN_NUMBER = 4;
static const unsigned THREAD_MAX_NUMBER = 16;

//* __Ty会被直接保存在 vector中 最好__Ty可以作为指针类型存在
class Thread_Pool
{
public:
    Thread_Pool(unsigned thread_number)
    {
        if (thread_number < THREAD_MIN_NUMBER)
            thread_number = THREAD_MIN_NUMBER;
        if (thread_number > THREAD_MAX_NUMBER)
            thread_number = THREAD_MAX_NUMBER;
        __worker_number = thread_number;
    }

    void Start() // 创建线程 线程池的启动函数
    {
        if (!__threadpool_is_stop.load())
        {
            LOG(WARNING, "thread pool has been already started!");
            return;
        }
        assert(__worker_number >= THREAD_MIN_NUMBER && __worker_number <= THREAD_MAX_NUMBER); // just make sure
        auto work = [this]()
        {
            Thread_Pool_Continent_Base *job;
            for (;;)
            {
                {
                    std::unique_lock<std::mutex> tmp_lock(locker);
                    __c_variable.wait(tmp_lock, [this]
                                      { return this->__threadpool_is_stop || !this->__resource_queue.empty(); });
                    if (__resource_queue.empty())
                    {
                        break;
                    }
                    job = __resource_queue.back();
                    __resource_queue.pop_back();
                }           // job->Run(); 可能会比较耗时 控制一下🔒的粒度
                job->Run(); // 纯虚函数的调用
            }
        };
        for (unsigned i = 0; i < __worker_number; ++i)
            __threads_queue.emplace_back(work);
        __threadpool_is_stop.store(false);
    }

    void Submit(Thread_Pool_Continent_Base *continent)
    {
        if (__threadpool_is_stop.load())
        {
            LOG(ERROR, "you should start the thread pool first!");
            assert(false);
        }
        std::unique_lock<std::mutex> tmp_lock(locker);
        __resource_queue.emplace_back(continent);
        __c_variable.notify_one();
    }

    void Exit()
    {
        __threadpool_is_stop.store(true);
        __c_variable.notify_all();
        LOG(INFO, "thread_pool will exit!");
    }

    ~Thread_Pool()
    {
        Exit();
        for (auto &item : __threads_queue)
        {
            if (item.joinable())
                item.join();
        }
    }

    Thread_Pool() = delete;
    Thread_Pool(const Thread_Pool &) = delete;
    Thread_Pool(Thread_Pool &&) = delete;
    Thread_Pool &operator=(const Thread_Pool &) = delete;
    Thread_Pool &operator=(Thread_Pool &&) = delete;

private:
    std::atomic<bool> __threadpool_is_stop{true};
    std::vector<std::thread> __threads_queue;
    std::vector<Thread_Pool_Continent_Base *> __resource_queue;
    mutable std::mutex locker;
    std::condition_variable __c_variable;
    unsigned int __worker_number{8};
};

#endif