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
    m_epoll_base->start_living_system_io_object(m_io_object.get());
}

HttpsClientRequest::~HttpsClientRequest()
{
}
