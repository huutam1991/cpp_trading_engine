#include "tcp_connection.h"

TCPConnection::TCPConnection(EpollBase* epoll_base, const std::string& hostname, int port, std::function<void()> on_connect, std::function<void()> on_disconnect)
        :   m_epoll_base{epoll_base}, m_hostname{hostname}, m_port{port}, m_on_connect{on_connect}, m_on_disconnect{on_disconnect}
{
    connect();
}

std::string TCPConnection::get_hostname() const
{
    return m_hostname;
}

int TCPConnection::get_port() const
{
    return m_port;
}

bool TCPConnection::is_disconnected() const
{
    return m_io_object == nullptr;
}

void TCPConnection::write(std::string data)
{
    m_io_object->write(std::move(data));
}

Future<std::string> TCPConnection::wait_for_data()
{
    return Future<std::string>([this](Future<std::string>::FutureValue* value)
    {
        if (m_pending_data_queue.empty() == false)
        {
            // If there is pending data, return immediately
            value->set_value(std::move(m_pending_data_queue.front()));
            m_pending_data_queue.pop();

            return;
        }

        m_waiting_data_value = value;
    });
}

void TCPConnection::connect()
{
    m_io_object = std::make_unique<HttpsClientIO>(m_hostname, m_port);
    m_io_object->set_on_disconnect_callback([this]()
    {
        this->on_disconnect();
    });
    m_io_object->set_on_response_received_callback([this](const char* buffer, std::uint32_t size)
    {
        this->on_response_received(buffer, size);
    });
    m_epoll_base->start_living_system_io_object(m_io_object.get());

    if (m_on_connect != nullptr)
    {
        m_on_connect();
    }
}

Task<void> TCPConnection::re_connect()
{
    // Retry connection after 5 seconds
    co_await Timer::sleep_for(5000);
    connect();
}

void TCPConnection::on_disconnect()
{
    m_io_object = nullptr;
    re_connect().start_running_on(m_epoll_base);

    if (m_on_disconnect != nullptr)
    {
        m_on_disconnect();
    }
}

void TCPConnection::on_response_received(const char* buffer, std::uint32_t size)
{
    if (m_waiting_data_value != nullptr)
    {
        m_waiting_data_value->set_value(std::string(buffer, size));
        m_waiting_data_value = nullptr;
    }
    else
    {
        // No waiting future, save to pending queue
        m_pending_data_queue.push(std::string(buffer, size));
    }
}