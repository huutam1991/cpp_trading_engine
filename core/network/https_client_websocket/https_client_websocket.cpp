#include <enum_reflect/enum_reflect.h>

#include "https_client_websocket.h"
#include "websocket_frame_parser.h"

HttpsClientWebsocket::HttpsClientWebsocket(
    EpollBase* epoll_base,
    const std::string& hostname,
    int port,
    const std::string& path,
    std::function<Task<void>()> on_connect,
    std::function<Task<void>(std::string)> on_message,
    std::function<Task<void>()> on_disconnect,
    std::function<Task<void>()> on_close)
    : m_epoll_base{epoll_base},
      m_hostname{hostname},
      m_port{port},
      m_path{path},
      m_name{hostname + path},
      m_on_connect{std::move(on_connect)},
      m_on_message{std::move(on_message)},
      m_on_disconnect{std::move(on_disconnect)},
      m_on_close{std::move(on_close)},
      m_tcp_connection(std::make_unique<TCPConnection>(epoll_base, hostname, port, [this]() { this->on_tcp_connect(); }, [this]() { this->on_tcp_disconnect(); }))
{
}

void HttpsClientWebsocket::on_tcp_connect()
{
    spdlog::info("HttpsClientWebsocket::on_tcp_connect - Connected to {}:{}", m_name, m_tcp_connection->get_port());

    connect().start_running_on(m_epoll_base);
}

void HttpsClientWebsocket::on_tcp_disconnect()
{
    spdlog::error("HttpsClientWebsocket::on_tcp_disconnect - Disconnected from {}:{}", m_name, m_tcp_connection->get_port());

    m_rest_request = nullptr;
    m_websocket_session = nullptr;

    // Need to intendly destroy [m_send_ping_task]
    // [Tam need re-test]
    m_send_ping_task.destroy();

    if (m_on_disconnect != nullptr)
    {
        m_on_disconnect().start_running_on(m_epoll_base);
    }
}

bool HttpsClientWebsocket::is_websocket_connected() const
{
    return m_websocket_session != nullptr;
}

Task<void> HttpsClientWebsocket::connect()
{
    m_rest_request = std::make_unique<HttpsClientRequest>(m_epoll_base, m_hostname, m_port, std::move(m_tcp_connection));

    co_await send_switch_protocol_request();

    // Move tcp connection from [m_rest_request] to [m_websocket_session]
    m_websocket_session = std::make_unique<HttpsClientWebsocketSession>(
        m_epoll_base,
        m_name,
        std::move(m_rest_request->release_tcp_connection()),
        std::move(m_on_message));

    // Release REST request object
    m_rest_request = nullptr;

    // Call user's [m_on_connect] callback
    co_await m_on_connect();

    // Start sending ping at 15 seconds interval
    m_send_ping_task = send_ping_at_15_second_interval();
    m_send_ping_task.start_running_on(m_epoll_base);

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

    HttpsClientResponse response = co_await m_rest_request->get(m_path);

    if (response.status_code == 101)
    {
        spdlog::info("[Websocket] connection established: {}", m_name);

        std::string leftover_data = m_rest_request->get_leftover_data();
        if (leftover_data.empty() == false)
        {
            WebSocketFrameParser frame_parser;
            frame_parser.feed(leftover_data.data(), leftover_data.size());
            std::vector<WebSocketFrameParser::Frame> frames = frame_parser.parse_frames();

            for (const auto& frame : frames)
            {
                spdlog::info("[Websocket] Received leftover websocket frame for [{}]: opcode={}, payload={}", m_name, enum_reflect::enum_name(frame.opcode), frame.payload_as_string());
            }
        }
    }
    else
    {
        spdlog::error("[Websocket] Failed to establish websocket connection for [{}], status code: {}", m_name, response.status_code);
        co_return;
    }

    co_return;
}

Task<void> HttpsClientWebsocket::send_ping_at_15_second_interval()
{
    while (is_websocket_connected())
    {
        co_await Timer::sleep_for(15000);
        write_ping("");
    }

    co_return;
}

void HttpsClientWebsocket::write(std::string message)
{
    if (is_websocket_connected())
    {
        m_websocket_session->write(std::move(message));
    }
}

void HttpsClientWebsocket::write_ping(const std::string& payload)
{
    if (is_websocket_connected())
    {
        m_websocket_session->write_ping(payload);
    }
}

void HttpsClientWebsocket::write_pong(const std::string& payload)
{
    if (is_websocket_connected())
    {
        m_websocket_session->write_pong(payload);
    }
}