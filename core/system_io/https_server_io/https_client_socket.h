#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "http_client_socket.h"
#include <network/tls_wrapper/tls_wrapper.h>

struct HttpsClientSocket : public HttpClientSocket
{
    int server_fd;
    TlsWrapper* tls_wrapper = nullptr;

    void set_ssl_context(TlsContext* tls_context);

    // SystemIOObject's methods
    virtual int generate_fd() override;
    virtual int activate() override;
    virtual int handle_io_data() override;
    virtual void release() override;

    // Handle data methods
    virtual int read_buffer(char* const buffer);
    virtual void write_to_socket_io(const char* buffer, std::uint32_t size);
};

using HttpsClientSocketPool = CachePool<HttpsClientSocket, 100>;