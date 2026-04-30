#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "http_websocket_server.h"
#include "http_websocket_connection.h"

#define BACKLOG_SOCKET 125

HttpWebsocketServer::HttpWebsocketServer(int port_value) : port{port_value}
{
}

void HttpWebsocketServer::write_to_connection(int fd, std::string message)
{
    if (fd < 0)
    {
        spdlog::error("HttpWebsocketServer::write_to_connection - Invalid fd: {}", fd);
        return;
    }

    const std::size_t index = static_cast<std::size_t>(fd);
    if (index >= m_websocket_connections_by_fd.size())
    {
        spdlog::error("HttpWebsocketServer::write_to_connection - fd {} is out of bounds for m_websocket_connections_by_fd", fd);
        return;
    }

    HttpWebsocketConnection* connection = m_websocket_connections_by_fd[index];
    if (connection != nullptr)
    {
        connection->write_text(message);
    }
    else
    {
        spdlog::warn("HttpWebsocketServer::write_to_connection - No active connection found for fd {}", index);
    }
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
    establish_connection(connection);

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

void HttpWebsocketServer::establish_connection(HttpWebsocketConnection* connection)
{
    connection->refresh();
    connection->set_server_fd(fd);
    connection->set_callbacks(
        // on_connect callback
        [this](int fd, HttpWebsocketConnection* connection) -> Task<void>
        {
            m_websocket_connections_by_fd[fd] = connection;

            if (on_connect != nullptr)
            {
                co_await on_connect(fd);
            }

            co_return;
        },
        // on_message callback
        [this](int fd, std::string message) -> Task<void>
        {
            if (on_message != nullptr)
            {
                co_await on_message(fd, message);
            }

            co_return;
        },
        // on_disconnect callback
        [this](int fd) -> Task<void>
        {
            m_websocket_connections_by_fd[fd] = nullptr;

            if (on_disconnect != nullptr)
            {
                co_await on_disconnect(fd);
            }

            co_return;
        }
    );

    epoll_base->start_living_system_io_object(connection);
}