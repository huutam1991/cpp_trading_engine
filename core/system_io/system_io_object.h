#pragma once

#include <sys/epoll.h>

class EpollBase;

struct SystemIOObject
{
    int fd; // File descriptor
    EpollBase *epoll_base = nullptr;

    virtual int generate_fd() = 0;
    virtual int get_io_events() { return EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP; }
    virtual int activate() = 0;
    virtual int handle_read() = 0;
    virtual int handle_write() = 0;
    virtual void release() = 0;
};