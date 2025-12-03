#include "https_client_request.h"

#include <netdb.h>
#include <arpa/inet.h>

HttpsClientRequest::HttpsClientRequest(EpollBase* epoll_base, const std::string& hostname, int port)
    :   m_epoll_base{epoll_base},
        m_hostname{hostname},
        m_port{port}
{
    // Connect
    connect();
}

HttpsClientRequest::~HttpsClientRequest()
{
    spdlog::info("HttpsClientRequest::~HttpsClientRequest - Destroying HttpsClientRequest to {}:{}", m_hostname, m_port);
}

void HttpsClientRequest::connect()
{
    m_io_object = std::make_unique<HttpClientRequestIO>(m_hostname, m_port);
    m_io_object->set_on_disconnect_callback([this]()
    {
        this->on_disconnect();
    });
    m_epoll_base->start_living_system_io_object(m_io_object.get());
}

void HttpsClientRequest::on_disconnect()
{
    spdlog::error("HttpsClientRequest::on_disconnect - Disconnected from {}:{}", m_hostname, m_port);
    m_io_object = nullptr;

    re_connect().start_running_on(m_epoll_base);
}

Task<void> HttpsClientRequest::re_connect()
{
    // Retry connection after 5 seconds
    co_await Timer::sleep_for(5000);
    connect();
}
void HttpsClientRequest::send_get_request(const std::string& path)
{
    std::string payload;
    payload  = "GET " + path + " HTTP/1.1\r\n";
    payload += "Host: " + m_hostname + "\r\n";
    payload += "Connection: close\r\n";
    payload += "User-Agent: RawEpollClient\r\n";
    payload += "Accept: */*\r\n";
    payload += "\r\n";

    m_io_object->write_to_socket_io(payload.c_str(), payload.size());
}