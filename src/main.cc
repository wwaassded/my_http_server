#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <log_what.hpp>
#include <socket_op.hpp>
#include <timer.hpp>

static const short localhost_port = 3308;
static const char *server_ip_address = "127.0.0.1";
static const int EPOLL_SIZE = 10000;
static const int TIME_SLOT = 5; // 定时器的出发间隔
static int pip[2];              // 定时器实现的通信工具 简单约定 1:为写端 0:为读端

// TODO: 如何实现定时 以及回调函数

void init_log_system(int argc, char *argv[])
{
    what::Log::Init(argc, argv);
    what::Log::flush_interval_ms = 100;
    what::Log::Add_file("test.log", what::Log::FileMode::Append, what::Log::Verbosity::VerbosityMESSAGE);
    what::Log::Set_thread_name("main thread");
}

void sig_handler(int sig) // 信号的回调函数 SIGALARM SIGTERM 定时/结束信号
{
    alarm(TIME_SLOT);
    if (sig == SIGALRM)
        write(pip[1], "SIGALARM", 8);
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
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        perror("socket()");
    }
    add_sig_handler(SIGTERM, sig_handler);
    Epoll __epoll(EPOLL_SIZE);
    set_socket_nonblock(listener);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(localhost_port);
    server_address.sin_addr.s_addr = inet_addr(server_ip_address);
    bind(listener, reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address));
    listen(listener, 5);
    __epoll.Add_fd(listener, MODE::LEVEL, false); // 处理用户连接使用水平触发 并不会设置 epolloneshot 防止丢失用户链接
    socketpair(AF_UNIX, SOCK_STREAM, 0, pip);
    set_socket_nonblock(pip[1]);
    __epoll.Add_fd(pip[0], MODE::LEVEL, false);
    add_sig_handler(SIGALRM, sig_handler); // 开始监听SIGALRM信号
    alarm(TIME_SLOT);                      // 开启定时器
    for (;;)
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
            if (socket == listener)
            {
                // 处理新的链接
                // 添加到线程池中
                // 添加 定时装置
            }
            else if (socket == pip[0])
            {
                // 定时事件发生 处理所有的超时链接
            }
            else if ((event->events | EPOLLERR) || (event->events | EPOLLHUP) || (event->events | EPOLLRDHUP))
            {
                // 出现错误的链接 服务端关闭对应的链接
            }
            else if (event->events | EPOLLIN)
            {
                // 处理 读事件
            }
            else if (event->events | EPOLLOUT)
            {
                // 处理写事件
            }
        }
    }
}