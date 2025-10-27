#pragma once

#include "http_client_socket.h"

struct HttpsClientSocket : public HttpClientSocket
{
    int server_fd;
    std::string save_buffer;

    void clear();

    // SystemIOObject's methods
    virtual int generate_fd();
    virtual int handle_io_data();

    // Handle data methods
    virtual int read_buffer(char* const buffer);
    virtual void write_to_socket_io(const char* buffer, std::uint32_t size);
    virtual void release();
};

using HttpsClientSocketPool = CachePool<HttpsClientSocket, 100>;