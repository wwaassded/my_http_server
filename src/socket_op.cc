#include <sys/fcntl.h>
#include <cassert>
#include "../include/socket_op.hpp"

int set_socket_nonblock(int socket_num)
{
    int old_opt = fcntl(socket_num, F_GETFD);
    old_opt = fcntl(socket_num, F_SETFD, old_opt | O_NONBLOCK);
    return old_opt;
}

int Epoll::Add_fd(int fd, MODE mode, bool is_oneshot)
{
    epoll_event new_event;
    new_event.data.fd = fd; // 对于 回调函数 会 很有用
    new_event.events = EPOLLIN;
    if (mode == MODE::EDGE)
        new_event.events |= EPOLLET;
    if (is_oneshot)
        new_event.events |= EPOLLONESHOT;
    assert(epoll_fd >= 0);
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &new_event);
}

auto Epoll::operator[](int pos) -> epoll_event *
{
    assert(pos >= 0 && pos < epoll_size);
    return &new_events[pos];
}

int Epoll::Epoll_Wait(int time)
{
    return epoll_wait(epoll_fd, new_events, epoll_size, time);
}

int Epoll::ReAdd_fd(int fd, uint32_t ev, bool is_ET)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    if (is_ET)
        event.events |= EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}