#pragma once

#include <memory>

#include <coroutine/epoll_base.h>
#include <system_io/https_websocket_server_io/https_websocket_server_io.h>

#include "https_websocket_server_route.h"

class HttpsWebsocketServer
{
    std::unique_ptr<HttpsWebsocketServerIO> m_https_websocket_server_io = nullptr;
    std::array<std::unique_ptr<HttpsWebsocketServerRoute>, 20> m_routes{nullptr};
    std::array<HttpsWebsocketServerRoute*, MAX_WEBSOCKET_CONNECTIONS> m_websocket_routes_by_fd{nullptr};

public:
    HttpsWebsocketServer(int port, EpollBase* epoll_base);

    void add_route(std::unique_ptr<HttpsWebsocketServerRoute> route);
    void write_to_connection(int fd, std::string message);
};