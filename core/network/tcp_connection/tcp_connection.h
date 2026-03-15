#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <queue>

#include <coroutine/epoll_base.h>
#include <coroutine/task.h>
#include <coroutine/future.h>
#include <time/timer.h>
#include <network/tls_wrapper/tls_wrapper.h>
#include <system_io/https_client_request_io/https_client_request_io.h>

class TCPConnection
{
    EpollBase* m_epoll_base = nullptr;
    std::string m_hostname;
    int m_port;
    std::unique_ptr<HttpsClientIO> m_io_object = nullptr;
    std::function<void()> m_on_connect = nullptr;
    std::function<void()> m_on_disconnect = nullptr;
    Future<std::string>::FutureValue* m_waiting_data_value = nullptr;
    std::queue<std::string> m_pending_data_queue;

public:
    TCPConnection() = default;

    TCPConnection(EpollBase* epoll_base, const std::string& hostname, int port, std::function<void()> on_connect, std::function<void()> on_disconnect);
    ~TCPConnection() = default;

    std::string get_hostname() const;
    int get_port() const;

    bool is_disconnected() const;
    void write(std::string data);
    Future<std::string> wait_for_data();

private:
    void connect();
    Task<void> re_connect();
    void on_disconnect();
    void on_response_received(const char* buffer, std::uint32_t size);
};