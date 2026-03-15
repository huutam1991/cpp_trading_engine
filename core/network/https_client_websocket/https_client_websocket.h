#pragma once

#include <network/tcp_connection/tcp_connection.h>
#include <network/https_client_request/https_client_request.h>

#include "https_client_websocket_session.h"

class HttpsClientWebsocket
{
    EpollBase* m_epoll_base = nullptr;
    std::string m_hostname;
    int m_port;

    // User's callbacks
    std::function<Task<void>()> m_on_connect = nullptr;
    std::function<Task<void>(std::string)> m_on_message = nullptr;
    std::function<Task<void>()> m_on_disconnect = nullptr;
    std::function<Task<void>()> m_on_close = nullptr;

    std::unique_ptr<TCPConnection> m_tcp_connection = nullptr;
    std::unique_ptr<HttpsClientRequest> m_rest_request = nullptr;
    std::unique_ptr<HttpsClientWebsocketSession> m_websocket_session = nullptr;

public:
    HttpsClientWebsocket(
        EpollBase* epoll_base,
        const std::string& hostname,
        int port,
        std::function<Task<void>()> on_connect,
        std::function<Task<void>(std::string)> on_message,
        std::function<Task<void>()> on_disconnect,
        std::function<Task<void>()> on_close);

private:
    Task<void> connect();
    Task<void> send_switch_protocol_request();

    void on_tcp_connect();
    void on_tcp_disconnect();
};