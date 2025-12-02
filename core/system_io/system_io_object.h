#pragma once

class EpollBase;

struct SystemIOObject
{
    int fd; // File descriptor
    EpollBase *epoll_base = nullptr;

    virtual int generate_fd() = 0;
    virtual void activate() = 0;
    virtual int handle_io_data() = 0;
    virtual void release() = 0;
};