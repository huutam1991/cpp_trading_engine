#include "https_client_websocket_connection.h"

HttpsClientWebsocketConnection::HttpsClientWebsocketConnection(EpollBase* epoll_base, const std::string& hostname, int port)
    : HttpsClientRequest(epoll_base, hostname, port)
{
}

void HttpsClientWebsocketConnection::on_disconnect()
{
}

void HttpsClientWebsocketConnection::on_response_received(const char* buffer, std::uint32_t size)
{
}
