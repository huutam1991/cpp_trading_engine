#pragma once

#include <string>

#include <coroutine/epoll_base.h>
#include <coroutine/task.h>
#include <network/tcp_connection/tcp_connection.h>

#include "websocket_frame_parser.h"

class HttpsClientWebsocketSession
{
    std::unique_ptr<TCPConnection> m_tcp_connection = nullptr;

public:
    HttpsClientWebsocketSession(EpollBase* epoll_base, const std::string& hostname, int port);
    ~HttpsClientWebsocketSession() = default;

    void use_tcp_connection(std::unique_ptr<TCPConnection> tcp_connection);

private:
    WebSocketFrameParser m_response_parser;

    Task<void> m_wait_for_tcp_data_task = nullptr;
    Task<void> wait_for_tcp_data();
};