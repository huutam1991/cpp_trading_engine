#pragma once

#include <network/https_websocket_server/https_websocket_server_route.h>
#include <network/https_websocket_server/https_websocket_server.h>

class WebsocketOrderbookRoute : public HttpsWebsocketServerRoute
{
public:
    WebsocketOrderbookRoute() : HttpsWebsocketServerRoute(WebsocketRouteName::orderbook) {}

    virtual Task<void> on_connect(int fd) override;
    virtual Task<void> on_message(int fd, std::string message) override;
    virtual Task<void> on_disconnect(int fd) override;
};