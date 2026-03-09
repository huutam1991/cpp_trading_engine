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

template <class T>
class TCPConnection
{
    EpollBase* m_epoll_base = nullptr;
    std::string m_hostname;
    int m_port;
    std::unique_ptr<HttpClientRequestIO> m_io_object = nullptr;
    T m_connection_type;

public:
    TCPConnection() = default;

    TCPConnection(EpollBase* epoll_base, const std::string& hostname, int port)
        :   m_epoll_base{epoll_base}, m_hostname{hostname}, m_port{port}
    {
        connect();
    }

    TCPConnection(const TCPConnection<U>& copy)
    {
        m_io_object = std::move(copy.m_io_object);
        m_epoll_base = copy.m_epoll_base;
        m_hostname = copy.m_hostname;
        m_port = copy.m_port;
    }

    ~TCPConnection() = default;

private:
    void connect()
    {
        m_io_object = std::make_unique<HttpClientRequestIO>(m_hostname, m_port);
        m_io_object->set_on_disconnect_callback([this]()
        {
            this->on_disconnect();
        });
        m_io_object->set_on_response_received_callback([this](const char* buffer, std::uint32_t size)
        {
            this->on_response_received(buffer, size);
        });
        m_epoll_base->start_living_system_io_object(m_io_object.get());
    }

    Task<void> re_connect()
    {
        // Retry connection after 5 seconds
        co_await Timer::sleep_for(5000);
        connect();
    }

    void on_disconnect()
    {
        m_io_object = nullptr;
        connection_type.on_disconnect();

        re_connect().start_running_on(m_epoll_base);
    }

    void on_response_received(const char* buffer, std::uint32_t size)
    {
        connection_type.on_response_received(buffer, size);
    }

    T& get_connection_type()
    {
        return m_connection_type;
    }
};