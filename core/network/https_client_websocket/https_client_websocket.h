#pragma once

#include "https_client_websocket_connection.h"

class HttpsClientWebsocket
{
    HttpsClientWebsocketConnection m_connection;

    HttpsClientWebsocket(EpollBase* epoll_base, const std::string& hostname, int port);
};