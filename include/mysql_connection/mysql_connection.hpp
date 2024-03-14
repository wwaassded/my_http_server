#ifndef MYSQL_CONNECTION_HPP
#define MYSQL_CONNECTION_HPP

#include <mutex>
#include <condition_variable>
#include <mysql/mysql.h>
#include <string>
#include <atomic>
#include <memory>
#include <vector>

using std::string;

const int MAGIC_NUMBER = 114514;

struct Mysql
{
    unsigned int magic_number{MAGIC_NUMBER};
    MYSQL *mysql;
};

class Mysql_Connection_Pool
{
public:
    static Mysql_Connection_Pool *GetInstance()
    {
        static Mysql_Connection_Pool connection_pool;
        return &connection_pool;
    }
    bool Init(string url, string user_name, string passwd, string db_name, unsigned short port, int max_connection);

    Mysql *Get_Free_Connection(int64_t); // 等待(ms)

    bool Free_Connection(Mysql *);

    ~Mysql_Connection_Pool()
    {
        destroy();
    }

private:
    void destroy();

    Mysql_Connection_Pool() // 只进行简单的初始化
    {
        free_connection = 0;
        cur_connection = 0;
    }
    int free_connection;
    int cur_connection;
    int max;

    mutable std::mutex locker; // 可以对出口与入口分别加锁 提高链接池的并发度
    std::condition_variable c_v;
    std::atomic<bool> is_stopped{true};
    std::vector<Mysql *> mysqls;

    // 链接MySQL 需要的信息
    string url;
    string user_name;
    string passwd;
    string db_name;
    unsigned short port;
    uint64_t size_of_Mysql;
};

class Mysql_Connection_RAII
{
public:
    Mysql_Connection_RAII() = default;
    ~Mysql_Connection_RAII()
    {
        if (__mysql != nullptr)
        {
            __pool->Free_Connection(__mysql);
        }
    }

    Mysql *Get_Mysql(Mysql **ptr, Mysql_Connection_Pool *pool, int64_t wait_time)
    {
        *ptr = pool->Get_Free_Connection(wait_time);
        if (*ptr == nullptr)
            return nullptr;
        __mysql = *ptr;
        __pool = pool;
        return __mysql;
    }

private:
    Mysql *__mysql{nullptr};
    Mysql_Connection_Pool *__pool{nullptr};
};

#endif