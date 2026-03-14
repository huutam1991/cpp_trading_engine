#include "https_client_websocket.h"

HttpsClientWebsocket::HttpsClientWebsocket(EpollBase* epoll_base, const std::string& hostname, int port)
    : m_tcp_connection(std::make_unique<TCPConnection>(epoll_base, hostname, port)),
      m_rest_request(epoll_base, hostname, port, std::move(m_tcp_connection)),
      m_connection{epoll_base, hostname, port}
{
    connect().start_running_on(epoll_base);
}

Task<void> HttpsClientWebsocket::connect()
{
    co_await send_switch_protocol_request();

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

    m_rest_request.add_header("Upgrade", "websocket");
    m_rest_request.add_header("Connection", "Upgrade");
    m_rest_request.add_header("Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ==");
    m_rest_request.add_header("Sec-WebSocket-Version", "13");

    HttpsClientResponse response = co_await m_rest_request.get("/");

    if (response.status_code == 101)
    {
        spdlog::info("Websocket connection established");
    }
    else
    {
        spdlog::error("Failed to establish websocket connection, status code: {}", response.status_code);
        co_return;
    }

    co_return;
}