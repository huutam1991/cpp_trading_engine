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
    std::unique_ptr<HttpClientRequestIO> m_io_object = nullptr;
    std::function<void()> m_on_disconnect = nullptr;
    Future<std::string>::FutureValue* m_waiting_data_value = nullptr;

public:
    TCPConnection() = default;

    TCPConnection(EpollBase* epoll_base, const std::string& hostname, int port)
        :   m_epoll_base{epoll_base}, m_hostname{hostname}, m_port{port}
    {
        connect();
    }

    ~TCPConnection() = default;

    bool is_disconnected()
    {
        return m_io_object == nullptr;
    }

    void write(std::string data)
    {
        m_io_object->write(std::move(data));
    }

    Future<std::string> wait_for_data()
    {
        return Future<std::string>([this](Future<std::string>::FutureValue* value)
        {
            m_waiting_data_value = value;
        });
    }

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
        re_connect().start_running_on(m_epoll_base);

        if (m_on_disconnect != nullptr)
        {
            m_on_disconnect();
        }
    }

    void on_response_received(const char* buffer, std::uint32_t size)
    {
        if (m_waiting_data_value != nullptr)
        {
            m_waiting_data_value->set_value(std::string(buffer, size));
            m_waiting_data_value = nullptr;
        }
    }
};