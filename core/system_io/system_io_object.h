#pragma once

class EpollBase;

struct SystemIOObject
{
    int fd; // File descriptor
    EpollBase *epoll_base = nullptr;

    virtual void generate_fd() = 0;
    virtual int handle_io_data() = 0;
};