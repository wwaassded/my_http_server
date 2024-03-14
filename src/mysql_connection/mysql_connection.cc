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
    for (int i = 0; i < free_connection; ++i)
    {
        MYSQL *conn;
        conn = mysql_init(conn);
        if (conn == nullptr)
            break;
        conn = mysql_real_connect(conn, url.c_str(), user_name.c_str(), passwd.c_str(), db_name.c_str(), port, nullptr, 0);
        if (conn == nullptr)
            break;
        mysqls[i].mysql = conn;
        ++free_connection;
    }
    if (free_connection != max_connection)
        return false;
    is_stopped.store(false);
    return true;
}

Mysql *Mysql_Connection_Pool::Get_Free_Connection(int64_t block_time)
{
}

bool Mysql_Connection_Pool::Free_Connection(Mysql *)
{
}