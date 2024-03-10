#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/epoll.h>

enum class MODE
{
    LEVEL = 0,
    EDGE = 0,
};

class Epoll
{
private:
    int epoll_fd;
    int epoll_size;
    epoll_event *new_events;

public:
    Epoll(int event_max_number) : epoll_fd(epoll_create1(0)), epoll_size(event_max_number), new_events(new epoll_event[epoll_size]) {}

    ~Epoll() { delete[] new_events; }

    int Add_fd(int fd, MODE mode, bool is_oneshot);

    int Epoll_Wait(int time);

    auto operator[](int pos) -> epoll_event *;
};

int set_socket_nonblock(int socket_num);

#endif