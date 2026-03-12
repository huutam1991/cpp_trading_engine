#pragma once

#include <network/tcp_connection/tcp_connection.h>
#include <network/https_client_request/https_client_request.h>

#include "https_client_websocket_connection.h"

class HttpsClientWebsocket
{
    std::unique_ptr<TCPConnection> m_tcp_connection = nullptr;
    HttpsClientRequest m_rest_request;
    HttpsClientWebsocketSession m_connection;

public:
    HttpsClientWebsocket(EpollBase* epoll_base, const std::string& hostname, int port);

private:
    Task<void> connect();
    Task<void> send_switch_protocol_request();
};