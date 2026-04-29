#pragma once

#include "http_websocket_server.h"
#include <network/tls_wrapper/tls_wrapper.h>

struct HttpsWebsocketServer : public HttpWebsocketServer
{
    TlsContext* server_ctx;

    HttpsWebsocketServer(
        int port_value,
        std::function<Task<void>(int)> on_connect_callback = nullptr,
        std::function<Task<void>(int, std::string)> on_message_callback = nullptr,
        std::function<Task<void>(int)> on_disconnect_callback = nullptr);

    virtual int generate_fd() override;
    virtual int handle_read() override;
};