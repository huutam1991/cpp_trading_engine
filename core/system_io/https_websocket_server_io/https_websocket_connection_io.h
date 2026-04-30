#pragma once

#include "http_websocket_connection_io.h"
#include <network/tls_wrapper/tls_wrapper.h>

struct HttpsWebsocketConnectionIO : public HttpWebsocketConnectionIO
{
    TlsWrapper* tls_wrapper = nullptr;

    void set_ssl_context(TlsContext* ctx)
    {
        tls_wrapper = new TlsWrapper(ctx);
    }

    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;

    virtual int read_buffer(char* const buffer, std::size_t size) override;
    virtual int write_to_socket_io(const char* buffer, std::uint32_t size) override;
};

using HttpsWebsocketConnectionIOPool = CachePool<HttpsWebsocketConnectionIO, 100>;