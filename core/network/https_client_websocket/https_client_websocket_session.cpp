#include "https_client_websocket_session.h"

HttpsClientWebsocketSession::HttpsClientWebsocketSession(EpollBase* epoll_base, const std::string& hostname, int port)
{
}

void HttpsClientWebsocketSession::use_tcp_connection(std::unique_ptr<TCPConnection> tcp_connection)
{
    m_tcp_connection = std::move(tcp_connection);
}

Task<void> HttpsClientWebsocketSession::wait_for_tcp_data()
{
    while (true)
    {
        std::string data = co_await m_tcp_connection->wait_for_data();

        m_response_parser.feed(data.data(), data.size());
        std::vector<WebSocketFrameParser::Frame> frames = m_response_parser.parse_frames();

    }

    co_return;
}