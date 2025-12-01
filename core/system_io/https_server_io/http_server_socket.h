#pragma once

#include <spdlog/spdlog.h>
#include <coroutine/epoll_base.h>
#include <system_io/system_io_object.h>

struct HttpServerSocket : public SystemIOObject
{
    int port;

    HttpServerSocket(int port_value) : port{port_value} {}

    // SystemIOObject's methods
    virtual int generate_fd();
    virtual int handle_io_data();
    virtual void release();
};