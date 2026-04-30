#pragma once

#include "http_websocket_server_io.h"
#include <network/tls_wrapper/tls_wrapper.h>

struct HttpsWebsocketServerIO : public HttpWebsocketServerIO
{
    TlsContext* server_ctx;

    HttpsWebsocketServerIO(int port_value);

    virtual int generate_fd() override;
    virtual int handle_read() override;
};