#pragma once

#include <network/https_client_request/https_client_request.h>

class HttpsClientWebsocketConnection : public HttpsClientRequest
{
public:
    HttpsClientWebsocketConnection(EpollBase* epoll_base, const std::string& hostname, int port);
    ~HttpsClientWebsocketConnection() = default;

protected:
    virtual void on_disconnect();
    virtual void on_response_received(const char* buffer, std::uint32_t size);
};