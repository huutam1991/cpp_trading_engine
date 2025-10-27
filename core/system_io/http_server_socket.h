#pragma once

#include "system_io_object.h"

struct HttpServerSocket : public SystemIOObject
{
    int port;

    HttpServerSocket(int port_value) : port{port_value} {}

    // SystemIOObject's methods
    virtual void generate_fd();
    virtual int handle_io_data();
};