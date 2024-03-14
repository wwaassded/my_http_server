#ifndef THREAD_POOL_CONTINENT_BASE_HPP
#define THREAD_POOL_CONTINENT_BASE_HPP

#include "../mysql_connection/mysql_connection.hpp"

//! 所有想要添加到线程池中的类型都应该继承自该基类
class Thread_Pool_Continent_Base
{
public:
    virtual void Run() = 0;
    Mysql *mysql{nullptr};
};

#endif
