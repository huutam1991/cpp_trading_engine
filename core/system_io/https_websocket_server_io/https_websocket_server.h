#pragma once

#include "http_websocket_server.h"
#include <network/tls_wrapper/tls_wrapper.h>

struct HttpsWebsocketServer : public HttpWebsocketServer
{
    TlsContext* server_ctx;

    HttpsWebsocketServer(int port_value);

    virtual int generate_fd() override;
    virtual int handle_read() override;
};