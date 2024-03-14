#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include "../include/log/log_what.hpp"
#include "../include/socket_op.hpp"
#include "../include/http_connection/http_connection.hpp"
#include "../include/timer/timer.hpp"
#include "../include/thread_pool/thread_pool.hpp"
#include "../include/mysql_connection/mysql_connection.hpp"

static const int THREAD_NUMBER = 8;
static const short localhost_port = 3308;
static const char *server_ip_address = "127.0.0.1";
static const int EPOLL_SIZE = 10000;
static const int TIME_SLOT = 5;                // 定时器的出发间隔
static int pip[2];                             // 定时器实现的通信工具 简单约定 1:为写端 0:为读端
static Timer global_timer(new __timer_node()); // 定时器的实现 有序链表
static std::map<int, Http_Connection> client_connections;
static std::map<int, User_Data *> client_datas;
static bool is_timeout = false;
static bool is_terminated = false;
static Epoll __epoll(EPOLL_SIZE);
static Thread_Pool thread_pool(THREAD_NUMBER);

// TODO：线程池 easy
// TODO：http链接 应该保留有一定的状态 hard

// TODO: __timer_node 的回调函数要写一些什么
void timer_call_back_function(void *user_data)
{
    auto data = reinterpret_cast<__user_data *>(user_data);
    close(data->__fd);
    // __epoll.Remove_fd(data->__fd);
    delete data;
    data = nullptr;
}

void init_log_system(int argc, char *argv[])
{
    what::Log::Init(argc, argv);
    what::Log::flush_interval_ms = 100;
    what::Log::Add_file("test.log", what::Log::FileMode::Append, what::Log::Verbosity::VerbosityMESSAGE);
    what::Log::Set_thread_name("main thread");
}

void timeout_handle()
{
    global_timer.Tick(); // Tick调用定时器的回调函数
    alarm(TIME_SLOT);
}

void sig_handler(int sig) // 信号的回调函数 SIGALARM SIGTERM 定时/结束信号
{
    if (sig == SIGALRM)
    {
        write(pip[1], "SIGALARM", 8);
    }
    else if (sig == SIGTERM)
    {
        write(pip[1], "SIGTERM", 7);
    }
}

void add_sig_handler(int sig, void (*handler)(int))
{
    struct sigaction new_action;
    memset(&new_action, '\0', sizeof(new_action));
    sigfillset(&new_action.sa_mask);
    new_action.sa_handler = handler;
    assert(sigaction(sig, &new_action, NULL) != -1);
}

static int listener;

int main(int argc, char *argv[])
{
    init_log_system(argc, argv);
    Mysql_Connection_Pool::GetInstance()->Init("localhost", "root", "2994899015", "test", 3306, 8);
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        perror("socket()");
    }
    add_sig_handler(SIGTERM, sig_handler);
    set_socket_nonblock(listener);
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(localhost_port);
    server_address.sin_addr.s_addr = inet_addr(server_ip_address); // inet_addr会自动的转换成网络字节序
    bind(listener, reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address));
    listen(listener, 5);
    __epoll.Add_fd(listener, MODE::LEVEL, false); // 处理用户连接使用水平触发 并不会设置 epolloneshot 防止丢失用户链接
    socketpair(AF_UNIX, SOCK_STREAM, 0, pip);
    set_socket_nonblock(pip[1]);
    __epoll.Add_fd(pip[0], MODE::LEVEL, false);
    thread_pool.Start();
    add_sig_handler(SIGALRM, sig_handler); // 开始监听SIGALRM信号
    alarm(TIME_SLOT);                      // 开启定时器

    while (!is_terminated)
    {
        int ready_fd_number = __epoll.Epoll_Wait(-1);
        if (ready_fd_number <= 0)
        {
            LOG(ERROR, "error from epoll");
            break;
        }
        for (int i = 0; i < ready_fd_number; ++i)
        {
            epoll_event *event = __epoll[i];
            int socket = event->data.fd;
            if (socket == listener) // listener 采用水平触发
            {
                //  处理新的链接
                sockaddr_in client_address;
                socklen_t address_size = sizeof(client_address);
                int client_sock = accept(listener, reinterpret_cast<sockaddr *>(&client_address), &address_size);
                if (client_sock <= 0)
                {
                    LOG(ERROR, "error from accept");
                    continue;
                }
                LOG(INFO, "IP:%s connected to our server!", inet_ntoa(client_address.sin_addr));
                set_socket_nonblock(client_sock);
                __epoll.Add_fd(client_sock, MODE::EDGE, true); // 边沿触发 加 epolloneshot
                // TODO: 完成http connection 结构体的设计后对 client_sock进行进一步的封装
                //   添加 到线程池中
                auto new_timer = new Timer_Node();
                new_timer->__expired_time = time(NULL) + TIME_SLOT; // 超时的时间
                new_timer->__expired_call_back_function = timer_call_back_function;
                auto data = new User_Data();
                data->__fd = client_sock;
                data->__client_address = client_address;
                data->__timer = new_timer;
                new_timer->__data = data;
                global_timer.Add_Timer_Node(new_timer);
                auto it = client_datas.emplace(client_sock, data);
                if (!it.second) // 插入失败 删除掉链接
                {
                    socket = client_sock;
                    LOG(ERROR, "failed to create a connect with %s", inet_ntoa(data->__client_address.sin_addr));
                    goto DELETE;
                }
            }
            else if (socket == pip[0] && (event->events | EPOLLIN))
            {
                // 有信号等待接收
                char buffer[32];
                bzero(buffer, sizeof(buffer));
                int ret = recv(socket, buffer, sizeof(buffer) - 1, 0);
                if (ret == -1)
                {
                    LOG(ERROR, "broken pipe!");
                    continue;
                }
                if (strcmp(buffer, "SIGALARM"))
                {
                    is_timeout = true;
                    break;
                }
                else if (strcmp(buffer, "SIGTERM"))
                {
                    is_terminated = true;
                }
            }
            else if ((event->events | EPOLLERR) || (event->events | EPOLLHUP) || (event->events | EPOLLRDHUP))
            {
            DELETE:
                // 出现错误的链接 服务端关闭对应的链接 删除掉 定时器
                User_Data *user_data{nullptr};
                auto it = client_datas.find(socket);
                if (it != client_datas.end())
                    user_data = it->second;
                if (user_data)
                {
                    if (!user_data->__timer)
                    {
                        global_timer.Remove_from_chain(user_data->__timer);
                        delete user_data->__timer;
                        user_data->__timer = nullptr;
                    }
                    close(user_data->__fd);
                    // __epoll.Remove_fd(user_data->__fd); //epoll应该会保证将close掉的fd移除监听队列
                    client_datas.erase(socket);
                    client_connections.erase(socket);
                    delete user_data;
                    user_data = nullptr;
                }
                else
                {
                    LOG(ERROR, "can not locate user data!");
                }
            }
            else if (event->events | EPOLLIN)
            {
                // 处理读事件
            }
            else if (event->events | EPOLLOUT)
            {
                // 处理写事件
            }
        }
        if (is_timeout)
        {
            // 有超时时间发生
            timeout_handle();
            is_timeout = false;
        }
    }
    // 释放掉server的所有资源
}