#pragma once

#include <network/tcp_connection/tcp_connection.h>
#include <network/https_client_request/https_client_request.h>

#include "https_client_websocket_session.h"

class HttpsClientWebsocket
{
    std::unique_ptr<TCPConnection> m_tcp_connection = nullptr;
    std::unique_ptr<HttpsClientRequest> m_rest_request;
    std::unique_ptr<HttpsClientWebsocketSession> m_websocket_session;

public:
    HttpsClientWebsocket(EpollBase* epoll_base, const std::string& hostname, int port);

private:
    Task<void> connect();
    Task<void> send_switch_protocol_request();

    void on_disconnect();
};