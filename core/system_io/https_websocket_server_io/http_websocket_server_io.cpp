#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "http_websocket_server_io.h"
#include "http_websocket_connection_io.h"

#define BACKLOG_SOCKET 125

HttpWebsocketServerIO::HttpWebsocketServerIO(int port_value) : port{port_value}
{
}

void HttpWebsocketServerIO::set_callbacks(
    std::function<Task<bool>(int, std::string, std::string)> on_connect_callback,
    std::function<Task<void>(int, std::string)> on_message_callback,
    std::function<Task<void>(int)> on_disconnect_callback)
{
    on_connect = std::move(on_connect_callback);
    on_message = std::move(on_message_callback);
    on_disconnect = std::move(on_disconnect_callback);
}

void HttpWebsocketServerIO::write_to_connection(int fd, std::string message)
{
    write_to_connection_task(fd, std::move(message)).start_running_on(epoll_base);
}

Task<void> HttpWebsocketServerIO::write_to_connection_task(int fd, std::string message)
{
    if (fd < 0)
    {
        spdlog::error("HttpWebsocketServerIO::write_to_connection - Invalid fd: {}", fd);
        co_return;

    }

    const std::size_t index = static_cast<std::size_t>(fd);
    if (index >= m_websocket_connections_by_fd.size())
    {
        spdlog::error("HttpWebsocketServerIO::write_to_connection - fd {} is out of bounds for m_websocket_connections_by_fd", fd);
        co_return;
    }

    HttpWebsocketConnectionIO* connection = m_websocket_connections_by_fd[index];
    if (connection != nullptr)
    {
        connection->write_text(message);
    }
    else
    {
        spdlog::warn("HttpWebsocketServerIO::write_to_connection - No active connection found for fd {}", index);
    }

    co_return;
}

int HttpWebsocketServerIO::generate_fd()
{
    sockaddr_in addr;
    int reuse = 1;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        spdlog::error("HttpWebsocketServerIO::generate_fd - socket failed: {}", std::strerror(errno));
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
    {
        spdlog::error("HttpWebsocketServerIO::generate_fd - setsockopt(SO_REUSEADDR) failed: {}", std::strerror(errno));
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
        spdlog::error("HttpWebsocketServerIO::generate_fd - bind failed on port {}: {}", port, std::strerror(errno));
        return -1;
    }

    if (listen(fd, BACKLOG_SOCKET) == -1)
    {
        spdlog::error("HttpWebsocketServerIO::generate_fd - listen failed on port {}: {}", port, std::strerror(errno));
        return -1;
    }

    spdlog::info("HttpWebsocketServerIO is listening on port {}", port);
    return fd;
}

int HttpWebsocketServerIO::activate()
{
    return 0;
}

int HttpWebsocketServerIO::handle_read()
{
    HttpWebsocketConnectionIO* connection = HttpWebsocketConnectionIOPool::acquire();
    establish_connection(connection);

    spdlog::debug("Size of HttpWebsocketConnectionIOPool = {}", HttpWebsocketConnectionIOPool::size());
    return 0;
}

int HttpWebsocketServerIO::handle_write()
{
    spdlog::warn("HttpWebsocketServerIO::handle_write - unexpected write event on server socket");
    return 0;
}

void HttpWebsocketServerIO::release()
{
}

void HttpWebsocketServerIO::establish_connection(HttpWebsocketConnectionIO* connection)
{
    connection->refresh();
    connection->set_server_fd(fd);
    connection->set_callbacks(
        // on_connect callback
        [this](int fd, HttpWebsocketConnectionIO* connection) -> Task<bool>
        {
            m_websocket_connections_by_fd[fd] = connection;

            if (on_connect != nullptr)
            {
                co_return co_await on_connect(fd, connection->get_path(), connection->get_bearer_token());
            }

            co_return false;
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