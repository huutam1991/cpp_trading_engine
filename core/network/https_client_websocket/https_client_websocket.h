#pragma once

#include <network/tcp_connection/tcp_connection.h>
#include <network/https_client_request/https_client_request.h>

#include "https_client_websocket_session.h"

class HttpsClientWebsocket
{
    EpollBase* m_epoll_base = nullptr;
    std::string m_hostname;
    int m_port;
    std::string m_path;
    std::string m_name;

    // User's callbacks
    std::move_only_function<Task<void>()> m_on_connect = nullptr;
    std::move_only_function<Task<void>(std::string)> m_on_message = nullptr;
    std::move_only_function<Task<void>()> m_on_disconnect = nullptr;
    std::move_only_function<Task<void>()> m_on_close = nullptr;

    std::unique_ptr<TCPConnection> m_tcp_connection = nullptr;
    std::unique_ptr<HttpsClientRequest> m_rest_request = nullptr;
    std::unique_ptr<HttpsClientWebsocketSession> m_websocket_session = nullptr;

public:
    HttpsClientWebsocket(
        EpollBase* epoll_base,
        const std::string& hostname,
        int port,
        const std::string& path,
        std::move_only_function<Task<void>()> on_connect,
        std::move_only_function<Task<void>(std::string)> on_message,
        std::move_only_function<Task<void>()> on_disconnect,
        std::move_only_function<Task<void>()> on_close);

    ~HttpsClientWebsocket();

    void write(std::string message);
    void write_ping(const std::string& payload = "");
    void write_pong(const std::string& payload = "");

private:
    bool is_websocket_connected() const;

    // Connect task
    Task<void> m_connect_task = nullptr;
    Task<void> connect();
    Task<void> send_switch_protocol_request();

    void on_tcp_connect();
    void on_tcp_disconnect();

    // For sending ping at 15 seconds interval
    Task<void> m_send_ping_task = nullptr;
    Task<void> send_ping_at_15_second_interval();
};