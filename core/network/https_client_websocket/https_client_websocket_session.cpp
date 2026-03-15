#include "https_client_websocket_session.h"

HttpsClientWebsocketSession::HttpsClientWebsocketSession(
    EpollBase* epoll_base,
    std::unique_ptr<TCPConnection> tcp_connection,
    std::function<Task<void>(std::string)> on_message)
    : m_tcp_connection(std::move(tcp_connection)),
      m_on_message(std::move(on_message))
{
    m_wait_for_tcp_data_task = wait_for_tcp_data();
    m_wait_for_tcp_data_task.start_running_on(epoll_base);
}

void HttpsClientWebsocketSession::write(std::string message)
{
    std::vector<char> frame = WebSocketFrameBuilder::build_text(message, true, true);
    std::string frame_str(frame.data(), frame.size());
    m_tcp_connection->write(frame_str);
}

Task<void> HttpsClientWebsocketSession::wait_for_tcp_data()
{
    while (true)
    {
        std::string data = co_await m_tcp_connection->wait_for_data();

        m_response_parser.feed(data.data(), data.size());
        std::vector<WebSocketFrameParser::Frame> frames = m_response_parser.parse_frames();

        for (const auto& frame : frames)
        {
            if (frame.opcode == WebSocketFrameParser::Opcode::Text)
            {
                co_await m_on_message(frame.payload_as_string());
            }
            else if (frame.opcode == WebSocketFrameParser::Opcode::Binary)
            {
                spdlog::info("Received WebSocket binary frame of size: {}", frame.payload.size());
            }
            else if (frame.opcode == WebSocketFrameParser::Opcode::Close)
            {
                spdlog::info("Received WebSocket close frame");
                co_return;
            }
            else if (frame.opcode == WebSocketFrameParser::Opcode::Ping)
            {
                spdlog::info("Received WebSocket ping frame");
            }
            else if (frame.opcode == WebSocketFrameParser::Opcode::Pong)
            {
                spdlog::info("Received WebSocket pong frame");
            }
        }
    }

    co_return;
}