#pragma once

#include "system_io_object.h"

struct ClientSocket : public SystemIOObject
{
    int server_fd;

    ClientSocket(int server_fd_value) : server_fd{server_fd_value} {}

    // SystemIOObject's methods
    virtual void generate_fd();
    virtual void handle_io_data();
};