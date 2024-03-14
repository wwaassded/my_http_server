#ifndef MYSQL_CONNECTION_HPP
#define MYSQL_CONNECTION_HPP

#include <mutex>
#include <condition_variable>
#include <mysql/mysql.h>
#include <list>
#include <string>
#include <atomic>
#include <memory>
#include "thread_safe_list.hpp"

using std::string;

const int MAGIC_NUMBER = 114514;

struct Mysql
{
    unsigned short int magic_number{MAGIC_NUMBER};
    MYSQL *mysql;
};

//!  TODO: get and free 有潜在死锁的风险
//? 解决方案  封装一个双端分别加锁的链表
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
    std::unique_ptr<Thread_Safe_List<Mysql>> mysqls{nullptr};
    mutable std::mutex locker; // 可以对出口与入口分别加锁 提高链接池的并发度
    std::condition_variable c_v;
    std::atomic<bool> is_stopped{true};

    // 链接MySQL 需要的信息
    string url;
    string user_name;
    string passwd;
    string db_name;
    unsigned short port;
    uint64_t size_of_Mysql;
};

#endif