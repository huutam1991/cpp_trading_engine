#pragma once

#include <spdlog/spdlog.h>
#include <coroutine/epoll_base.h>
#include <system_io/system_io_object.h>

struct HttpServerSocket : public NamedIOObject<HttpServerSocket>
{
    int port;

    HttpServerSocket(int port_value) : port{port_value} {}

    // SystemIOObject's methods
    virtual int generate_fd() override;
    virtual int get_io_events() override { return EPOLLIN; }
    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;
};