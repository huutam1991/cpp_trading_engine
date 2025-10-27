#include <iostream>
#include <string>
#include <cstring>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "client_socket.h"
#include <spdlog/spdlog.h>

void ClientSocket::generate_fd()
{
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1)
    {
        spdlog::info("HttpServer - accept: {}", std::strerror(errno));
    }
    else
    {
        spdlog::info("Connection to {}, established (fd = {})", inet_ntoa(client_addr.sin_addr), fd);

        int dwTimeout = 1000; // milliseconds
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&dwTimeout, sizeof dwTimeout);
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&dwTimeout, sizeof dwTimeout);

        int buffer_size = 1024 * 1024; // 1 MB
        setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
        setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

        int n;
        unsigned int m = sizeof(n);
        getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&n, &m);
        // spdlog::debug("fd = {}, Receive buffer = {}", fd, n);
        getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&n, &m);
        // spdlog::debug("fd = {}, Send buffer = {}", fd, n);
    }

    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void ClientSocket::handle_io_data()
{

}