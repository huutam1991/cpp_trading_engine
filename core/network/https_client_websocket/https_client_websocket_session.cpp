#include "https_client_websocket_session.h"

#include <spdlog/spdlog.h>

HttpsClientWebsocketSession::HttpsClientWebsocketSession(
    EpollBase* epoll_base,
    const std::string& name,
    std::unique_ptr<TCPConnection> tcp_connection,
    std::function<Task<void>(std::string)> on_message)
    : m_name(name),
      m_tcp_connection(std::move(tcp_connection)),
      m_on_message(std::move(on_message))
{
    m_wait_for_tcp_data_task = wait_for_tcp_data();
    m_wait_for_tcp_data_task.start_running_on(epoll_base);
}

HttpsClientWebsocketSession::~HttpsClientWebsocketSession()
{
    spdlog::debug("HttpsClientWebsocketSession::~HttpsClientWebsocketSession - Destroying Websocket Session [{}]", m_name);

    // Need to intendly destroy [m_wait_for_tcp_data_task]
    // [Tam need re-test]
    // m_wait_for_tcp_data_task.destroy(true);
}

void HttpsClientWebsocketSession::write_raw_frame(const std::vector<char>& frame)
{
    if (frame.empty())
    {
        return;
    }

    std::string frame_str(frame.data(), frame.size());

    if (m_tcp_connection)
    {
        m_tcp_connection->write(frame_str);
    }
}

void HttpsClientWebsocketSession::write(std::string message)
{
    write_raw_frame(WebSocketFrameBuilder::build_text(message, true, true));
}

void HttpsClientWebsocketSession::write_ping(const std::string& payload)
{
    write_raw_frame(WebSocketFrameBuilder::build_ping(payload, true));
}

void HttpsClientWebsocketSession::write_pong(const std::string& payload)
{
    write_raw_frame(WebSocketFrameBuilder::build_pong(payload, true));
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
                spdlog::debug("[Websocket] Received binary frame for [{}] of size: {}", m_name, frame.payload.size());
            }
            else if (frame.opcode == WebSocketFrameParser::Opcode::Close)
            {
                spdlog::debug("[Websocket] Received close frame for [{}]", m_name);

                // Good practice: echo close back before stopping.
                write_raw_frame(WebSocketFrameBuilder::build_close(true));

                co_return;
            }
            else if (frame.opcode == WebSocketFrameParser::Opcode::Ping)
            {
                spdlog::debug("[Websocket] Received ping frame for [{}], payload size: {}", m_name, frame.payload.size());

                // MUST reply pong with the same payload
                write_pong(frame.payload_as_string());
            }
            else if (frame.opcode == WebSocketFrameParser::Opcode::Pong)
            {
                spdlog::debug("[Websocket] Received pong frame for [{}], payload size: {}", m_name, frame.payload.size());
            }
        }
    }

    co_return;
}