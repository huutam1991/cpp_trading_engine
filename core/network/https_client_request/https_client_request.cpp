#include "https_client_request.h"

#include <netdb.h>
#include <arpa/inet.h>

HttpsClientRequest::HttpsClientRequest(EpollBase* epoll_base, const std::string& hostname, int port)
    :   m_epoll_base{epoll_base},
        m_hostname{hostname},
        m_port{port},
        m_io_object{std::make_unique<HttpClientRequestIO>(hostname, m_port)}
{
    // Connect
    m_io_object->set_on_disconnect_callback([this]()
    {
        this->on_disconnect();
    });
    m_epoll_base->start_living_system_io_object(m_io_object.get());
}

HttpsClientRequest::~HttpsClientRequest()
{
    m_io_object->set_on_disconnect_callback(nullptr);
}

void HttpsClientRequest::on_disconnect()
{
    spdlog::error("HttpsClientRequest::on_disconnect - Disconnected from {}:{}", m_hostname, m_port);

    m_io_object = std::make_unique<HttpClientRequestIO>(m_hostname, m_port);
    m_io_object->set_on_disconnect_callback([this]()
    {
        this->on_disconnect();
    });
    m_epoll_base->start_living_system_io_object(m_io_object.get());
}