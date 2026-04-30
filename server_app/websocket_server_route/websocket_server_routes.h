#pragma once

#include <network/https_websocket_server/https_websocket_server.h>

class WebsocketServerRoutes
{
    std::unique_ptr<HttpsWebsocketServer> m_server = nullptr;

public:
    WebsocketServerRoutes(int port, EpollBase* epoll_base);
};