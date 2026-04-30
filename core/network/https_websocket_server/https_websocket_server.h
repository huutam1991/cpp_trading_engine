#pragma once

#include <memory>

#include "https_websoket_server_route.h"
#include <system_io/https_websocket_server_io/https_websocket_server_io.h>

class HttpsWebsocketServer
{
    std::unique_ptr<HttpsWebsocketServerIO> m_https_websocket_server_io = nullptr;
    std::array<std::unique_ptr<HttpsWebsocketServerRoute>, 20> m_routes{nullptr};

public:
    HttpsWebsocketServer(int port);

    void add_route(WebsocketServerRouteEnum route_enum, std::unique_ptr<HttpsWebsocketServerRoute> route);
};