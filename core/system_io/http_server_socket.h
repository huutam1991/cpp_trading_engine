#pragma once

#include <spdlog/spdlog.h>
#include "system_io_object.h"
#include <coroutine/epoll_base.h>

struct HttpServerSocket : public SystemIOObject
{
    EpollBase* epoll_base;
    int port;

    HttpServerSocket(EpollBase* epoll_base_value, int port_value) : epoll_base{epoll_base_value}, port{port_value} {}

    // SystemIOObject's methods
    virtual void generate_fd();
    virtual int handle_io_data();
};