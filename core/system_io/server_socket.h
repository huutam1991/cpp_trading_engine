#pragma once

#include "system_io_object.h"

struct ServerSocket : public SystemIOObject
{
    int port;

    ServerSocket(int port_value) : port{port_value} {}

    // SystemIOObject's methods
    virtual void generate_fd();
    virtual void handle_io_data();
};