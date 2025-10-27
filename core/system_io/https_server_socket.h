#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "http_server_socket.h"

struct HttpsServerSocket : public HttpServerSocket
{
    int port;
    SSL_CTX* ctx;
    SSL* ssl;
    bool ssl_accept_success;

    SSL_CTX *create_context();
    void configure_context(SSL_CTX *ctx);

    HttpsServerSocket(int port_value);

    // SystemIOObject's methods
    virtual void generate_fd();
    virtual int handle_io_data();
    virtual void release();
};