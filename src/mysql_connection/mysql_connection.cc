#include "../../include/mysql_connection/mysql_connection.hpp"
#include <cassert>

bool Mysql_Connection_Pool::Init(string url, string user_name, string passwd, string db_name, unsigned short port, int max_connection)
{
    if (!is_stopped.load())
        return false;
    this->url = url;
    this->user_name = user_name;
    this->passwd = passwd;
    this->db_name = db_name;
    this->port = port;
    size_of_Mysql = sizeof(Mysql);
    std::unique_lock<std::mutex> tmp_lock(locker);
    max = max_connection;
    for (int i = 0; i < max_connection; ++i)
    {
        auto it = new Mysql;
        MYSQL *conn;
        conn = mysql_init(conn);
        if (conn == nullptr)
            break;
        conn = mysql_real_connect(conn, url.c_str(), user_name.c_str(), passwd.c_str(), db_name.c_str(), port, nullptr, 0);
        if (conn == nullptr)
            break;
        it->mysql = conn;
        mysqls.emplace_back(it);
        ++free_connection;
    }
    if (free_connection != max_connection)
        return false;
    is_stopped.store(false);
    return true;
}

Mysql *Mysql_Connection_Pool::Get_Free_Connection(int64_t block_time)
{
    std::unique_lock<std::mutex> tmp_locker(locker);
    auto predict = [this]
    {
        return this->free_connection != 0 || this->is_stopped;
    };
    if (block_time < 0)
        c_v.wait(tmp_locker, predict);
    else
        c_v.wait_for(tmp_locker, std::chrono::milliseconds(block_time), predict);
    if (is_stopped.load() || free_connection == 0)
        return nullptr;
    else
    {
        --free_connection;
        ++cur_connection;
        auto ret = std::move(mysqls.back());
        mysqls.pop_back();
        return ret;
    }
}

bool Mysql_Connection_Pool::Free_Connection(Mysql *cur)
{
    if (cur == nullptr || sizeof(*cur) != size_of_Mysql || cur->magic_number != MAGIC_NUMBER || max == free_connection || cur_connection == 0)
        return false;
    std::unique_lock<std::mutex> tmp_locker(locker);
    ++free_connection;
    --cur_connection;
    mysqls.emplace_back(cur);
    c_v.notify_one();
    return true;
}

void Mysql_Connection_Pool::destroy()
{
    is_stopped.store(true);
    c_v.notify_all();
    std::unique_lock<std::mutex> tmp_locker(locker);
    if (mysqls.size() > 0)
    {
        for (auto &item : mysqls)
        {
            mysql_close(item->mysql);
            delete item;
            item = nullptr;
        }
        free_connection = 0;
        cur_connection = 0;
        std::vector<Mysql *>().swap(mysqls);
    }
}