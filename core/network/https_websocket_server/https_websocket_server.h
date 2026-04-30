#pragma once

#include <memory>

#include "https_websoket_server_route.h"
#include <system_io/https_websocket_server_io/https_websocket_server_io.h>

class HttpsWebsocketServer
{
    std::unique_ptr<HttpsWebsocketServerIO> m_https_websocket_server_io = nullptr;

public:
    HttpsWebsocketServer(int port);
};