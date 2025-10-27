#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "http_client_socket.h"

enum SSL_ACCEPT_STATUS
{
    ERROR = -1,
    OK = 0,
    WANT_IO = 1
};

struct HttpsClientSocket : public HttpClientSocket
{
    int server_fd;
    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    bool ssl_accept_success = false;
    std::string save_buffer;

    void clear();
    void set_ssl_context(SSL_CTX* ctx_value);
    SSL_ACCEPT_STATUS do_ssl_accept();

    // SystemIOObject's methods
    virtual int generate_fd();
    virtual int handle_io_data();
    virtual void release();

    // Handle data methods
    virtual int read_buffer(char* const buffer);
    virtual void write_to_socket_io(const char* buffer, std::uint32_t size);
};

using HttpsClientSocketPool = CachePool<HttpsClientSocket, 100>;