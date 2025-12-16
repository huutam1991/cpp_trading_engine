#pragma once

#include <sys/epoll.h>
#include <utils/type_name.h>

class EpollBase;

struct SystemIOObject
{
    int fd; // File descriptor
    EpollBase *epoll_base = nullptr;

    virtual std::string name() = 0;
    virtual int generate_fd() = 0;
    virtual int get_io_events() { return EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP; }
    virtual int activate() = 0;
    virtual int handle_read() = 0;
    virtual int handle_write() = 0;
    virtual void release() = 0;
};

template <class T>
struct NamedIOObject : public SystemIOObject
{
    virtual std::string name() override
    {
        return type_name::TypeName<T>::name();
    }
};