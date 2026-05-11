#pragma once

#include <unordered_set>

#include <network/https_websocket_server/https_websocket_server_route.h>
#include <network/https_websocket_server/https_websocket_server.h>

class WebsocketOrderRoute : public HttpsWebsocketServerRoute
{
public:
    WebsocketOrderRoute();

    std::unordered_set<int> m_connected_fds;

    virtual Task<void> on_connect(int fd) override;
    virtual Task<void> on_message(int fd, std::string message) override;
    virtual Task<void> on_disconnect(int fd) override;
};