#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "http_server_socket.h"
#include <network/tls_wrapper/tls_wrapper.h>

struct HttpsServerSocket : public HttpServerSocket
{
    int port;
    TlsContext* server_ctx;

    HttpsServerSocket(int port_value);

    // SystemIOObject's methods
    virtual int generate_fd() override;
    virtual int activate() override;
    virtual int handle_io_data() override;
    virtual void release() override;
};