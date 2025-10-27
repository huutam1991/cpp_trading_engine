#include <iostream>
#include <string>
#include <cstring>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "https_server_socket.h"
#include "http_client_socket.h"

#define BACKLOG_SOCKET 125

void HttpsServerSocket::generate_fd()
{
    // Exactly the same as HttpServerSocket
    HttpServerSocket::generate_fd();
}

int HttpsServerSocket::handle_io_data()
{
    HttpClientSocket* client_socket = HttpClientSocketPool::acquire();
    client_socket->set_server_fd(fd);
    epoll_base->start_living_on(client_socket);

    spdlog::info("Size of HttpClientSocketPool = {}", HttpClientSocketPool::size());

    return 0;
}

void HttpsServerSocket::release()
{
    // Nothing to release for server socket
}