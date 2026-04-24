#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "http_websocket_server.h"
#include "http_websocket_connection.h"

#define BACKLOG_SOCKET 125

HttpWebsocketServer::HttpWebsocketServer(
    int port_value,
    std::function<Task<void>(int)> on_connect_callback,
    std::function<Task<void>(int, std::string)> on_message_callback,
    std::function<Task<void>(int)> on_disconnect_callback)
    :   port{port_value},
        on_connect{std::move(on_connect_callback)},
        on_message{std::move(on_message_callback)},
        on_disconnect{std::move(on_disconnect_callback)}
{
}

int HttpWebsocketServer::generate_fd()
{
    sockaddr_in addr;
    int reuse = 1;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        spdlog::error("HttpWebsocketServer::generate_fd - socket failed: {}", std::strerror(errno));
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
    {
        spdlog::error("HttpWebsocketServer::generate_fd - setsockopt(SO_REUSEADDR) failed: {}", std::strerror(errno));
        return -1;
    }

    int buffer_size = 1024 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        spdlog::error("HttpWebsocketServer::generate_fd - bind failed on port {}: {}", port, std::strerror(errno));
        return -1;
    }

    if (listen(fd, BACKLOG_SOCKET) == -1)
    {
        spdlog::error("HttpWebsocketServer::generate_fd - listen failed on port {}: {}", port, std::strerror(errno));
        return -1;
    }

    spdlog::info("HttpWebsocketServer is listening on port {}", port);
    return fd;
}

int HttpWebsocketServer::activate()
{
    return 0;
}

int HttpWebsocketServer::handle_read()
{
    HttpWebsocketConnection* connection = HttpWebsocketConnectionPool::acquire();
    connection->refresh();
    connection->set_server_fd(fd);
    connection->set_callbacks(on_connect, on_message, on_disconnect);
    epoll_base->start_living_system_io_object(connection);

    spdlog::debug("Size of HttpWebsocketConnectionPool = {}", HttpWebsocketConnectionPool::size());
    return 0;
}

int HttpWebsocketServer::handle_write()
{
    spdlog::warn("HttpWebsocketServer::handle_write - unexpected write event on server socket");
    return 0;
}

void HttpWebsocketServer::release()
{
}
