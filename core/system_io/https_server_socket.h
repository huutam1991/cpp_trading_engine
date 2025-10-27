#pragma once

#include "http_server_socket.h"

struct HttpsServerSocket : public HttpServerSocket
{
    int port;

    HttpsServerSocket(int port_value) : HttpServerSocket{port_value} {}

    // SystemIOObject's methods
    virtual void generate_fd();
    virtual int handle_io_data();
    virtual void release();
};