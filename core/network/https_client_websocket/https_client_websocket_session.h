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
    std::string m_name;

public:
    HttpsClientWebsocketSession(EpollBase* epoll_base, const std::string& name, std::unique_ptr<TCPConnection> tcp_connection, std::move_only_function<Task<void>(std::string)> on_message);
    ~HttpsClientWebsocketSession();

    void write(std::string message);
    void write_ping(const std::string& payload = "");
    void write_pong(const std::string& payload = "");

private:
    void write_raw_frame(const std::vector<char>& frame);

private:
    WebSocketFrameParser m_response_parser;
    std::move_only_function<Task<void>(std::string)> m_on_message;

    Task<void> m_wait_for_tcp_data_task = nullptr;
    Task<void> wait_for_tcp_data();
};