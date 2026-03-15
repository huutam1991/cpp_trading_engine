#pragma once

#include <string>

#include <coroutine/epoll_base.h>
#include <coroutine/task.h>
#include <network/tcp_connection/tcp_connection.h>

#include "websocket_frame_builder.h"
#include "websocket_frame_parser.h"

class HttpsClientWebsocketSession
{
    std::unique_ptr<TCPConnection> m_tcp_connection = nullptr;

public:
    HttpsClientWebsocketSession(EpollBase* epoll_base, std::unique_ptr<TCPConnection> tcp_connection, std::function<Task<void>(std::string)> on_message);
    ~HttpsClientWebsocketSession() = default;

    void write(std::string message);

private:
    WebSocketFrameParser m_response_parser;
    std::function<Task<void>(std::string)> m_on_message;

    Task<void> m_wait_for_tcp_data_task = nullptr;
    Task<void> wait_for_tcp_data();
};