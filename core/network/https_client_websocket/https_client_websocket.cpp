#include <enum_reflect/enum_reflect.h>

#include "https_client_websocket.h"
#include "websocket_frame_parser.h"

HttpsClientWebsocket::HttpsClientWebsocket(EpollBase* epoll_base, const std::string& hostname, int port)
    : m_epoll_base{epoll_base},
      m_hostname{hostname},
      m_port{port},
      m_tcp_connection(std::make_unique<TCPConnection>(epoll_base, hostname, port, [this]() { this->on_tcp_connect(); }, [this]() { this->on_tcp_disconnect(); }))
{
}

void HttpsClientWebsocket::on_tcp_connect()
{
    spdlog::info("HttpsClientWebsocket::on_tcp_connect - Connected to {}:{}", m_tcp_connection->get_hostname(), m_tcp_connection->get_port());
    connect().start_running_on(m_epoll_base);
}

void HttpsClientWebsocket::on_tcp_disconnect()
{
    spdlog::error("HttpsClientWebsocket::on_tcp_disconnect - Disconnected from {}:{}", m_tcp_connection->get_hostname(), m_tcp_connection->get_port());
}

Task<void> HttpsClientWebsocket::connect()
{
    m_rest_request = std::make_unique<HttpsClientRequest>(m_epoll_base, m_hostname, m_port, std::move(m_tcp_connection));

    co_await send_switch_protocol_request();

    // Move tcp connection from [m_rest_request] to [m_websocket_session]
    m_websocket_session = std::make_unique<HttpsClientWebsocketSession>(m_epoll_base, m_hostname, m_port);
    m_websocket_session->use_tcp_connection(std::move(m_rest_request->release_tcp_connection()));
    m_rest_request = nullptr; // Release REST request object

    co_return;
}

// Implementation for sending the switch protocol request
Task<void> HttpsClientWebsocket::send_switch_protocol_request()
{
    // wss://ws.ifelse.io

    // GET / HTTP/1.1
    // Host: ws.ifelse.io
    // Upgrade: websocket
    // Connection: Upgrade
    // Sec-WebSocket-Key: <random_base64>
    // Sec-WebSocket-Version: 13

    m_rest_request->add_header("Upgrade", "websocket");
    m_rest_request->add_header("Connection", "Upgrade");
    m_rest_request->add_header("Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ==");
    m_rest_request->add_header("Sec-WebSocket-Version", "13");

    HttpsClientResponse response = co_await m_rest_request->get("/");

    if (response.status_code == 101)
    {
        spdlog::info("Websocket connection established");

        std::string leftover_data = m_rest_request->get_leftover_data();
        if (leftover_data.empty() == false)
        {
            WebSocketFrameParser frame_parser;
            frame_parser.feed(leftover_data.data(), leftover_data.size());
            std::vector<WebSocketFrameParser::Frame> frames = frame_parser.parse_frames();

            for (const auto& frame : frames)
            {
                spdlog::info("Received leftover websocket frame: opcode={}, payload={}", enum_reflect::enum_name(frame.opcode), frame.payload_as_string());
            }
        }
    }
    else
    {
        spdlog::error("Failed to establish websocket connection, status code: {}", response.status_code);
        co_return;
    }

    co_return;
}