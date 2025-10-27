#include <iostream>
#include <string>
#include <cstring>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "server_socket.h"

#define BACKLOG_SOCKET 125

void ServerSocket::generate_fd()
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
    addr.sin_port = htons(m_port);
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
}