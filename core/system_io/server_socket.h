#pragma once

#include "system_io_object.h"

struct ServerSocket : public SystemIOObject
{
    int m_port;

    ServerSocket(int port) : m_port{port} {}

    // SystemIOObject's methods
    virtual void generate_fd();
    virtual void handle_io_data();
};