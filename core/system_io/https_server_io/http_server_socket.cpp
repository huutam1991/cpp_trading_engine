#include <iostream>
#include <string>
#include <cstring>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "http_server_socket.h"
#include "http_client_socket.h"

#define BACKLOG_SOCKET 125

int HttpServerSocket::generate_fd()
{
    sockaddr_in addr;
    int reuse = 1;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        // LOG(INFO) << "socket: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
    {
        // LOG(INFO) << "setsockopt: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    int buffer_size = 1024 * 1024; // 1 MB
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (sockaddr*) &addr, sizeof(sockaddr)) == -1)
    {
        // LOG(INFO) << "bind: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(fd, BACKLOG_SOCKET) == -1)
    {
        // LOG(INFO) << "listen: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        // spdlog::info("Http server is listening on port: {}", m_port);
    }

    return fd;
}

int HttpServerSocket::activate()
{
    // Nothing to do for server socket
    return 0;
}

int HttpServerSocket::handle_read()
{
    HttpClientSocket* client_socket = HttpClientSocketPool::acquire();
    client_socket->set_server_fd(fd);
    epoll_base->start_living_system_io_object(client_socket);

    spdlog::info("Size of HttpClientSocketPool = {}", HttpClientSocketPool::size());

    return 0;
}

int HttpServerSocket::handle_write()
{
    // Nothing to do for write event
    return 0;
}

void HttpServerSocket::release()
{
    // Nothing to release for server socket
}