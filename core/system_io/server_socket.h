#pragma once

#include "system_io_object.h"

struct ServerSocket : public SystemIOObject
{
    int m_port;

    ServerSocket(int port) : m_port{port} {}
    virtual void generate_fd();
};