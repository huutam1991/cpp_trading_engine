#include "https_client_websocket.h"

HttpsClientWebsocket::HttpsClientWebsocket(EpollBase* epoll_base, const std::string& hostname, int port)
    : m_connection{epoll_base, hostname, port}
{
}