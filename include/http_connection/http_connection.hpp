#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include <thread_pool_continent_base.hpp>

class Http_Connection : public Thread_Pool_Continent_Base
{
public:
    void Run() override;
};

#endif