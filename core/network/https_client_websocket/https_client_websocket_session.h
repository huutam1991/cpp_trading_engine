#pragma once

#include <string>

#include <coroutine/epoll_base.h>
#include <network/tcp_connection/tcp_connection.h>


class HttpsClientWebsocketSession
{
    std::unique_ptr<TCPConnection> m_tcp_connection = nullptr;

public:
    HttpsClientWebsocketSession(EpollBase* epoll_base, const std::string& hostname, int port);
    ~HttpsClientWebsocketSession() = default;

protected:
    virtual void on_disconnect();
    virtual void on_response_received(const char* buffer, std::uint32_t size);
};