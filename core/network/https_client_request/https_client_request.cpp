#include "https_client_request.h"

#include <netdb.h>
#include <arpa/inet.h>

HttpsClientRequest::HttpsClientRequest(EpollBase* epoll_base, const std::string& hostname, int port)
    :   m_epoll_base{epoll_base},
        m_tls_wrapper{std::make_unique<TlsWrapper>(get_tls_context())},
        m_hostname{hostname},
        m_port{port},
        m_ip{resolve_hostname()},
        m_io_object{m_ip, m_port, m_tls_wrapper.get()}
{
    // Set SNI
    SSL_set_tlsext_host_name(m_tls_wrapper->get_ssl(), m_hostname.c_str());

    // Connect
    m_epoll_base->start_living_system_io_object(&m_io_object);
}

HttpsClientRequest::~HttpsClientRequest()
{
}

TlsContext* HttpsClientRequest::get_tls_context()
{
    static TlsClientContext client_ctx{true, ""};
    return &client_ctx;
}

std::string HttpsClientRequest::resolve_hostname()
{
    addrinfo hints{};
    hints.ai_family   = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    addrinfo* result = nullptr;
    int ret = getaddrinfo(m_hostname.c_str(), nullptr, &hints, &result);
    if (ret != 0)
    {
        spdlog::error("HttpsClientRequest::resolve_hostname - getaddrinfo failed for {}: {}", m_hostname, gai_strerror(ret));
        return ""; // fail
    }

    char ip_str[INET_ADDRSTRLEN] = {0};

    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, sizeof(ip_str));
    std::string ip = ip_str;

    freeaddrinfo(result);

    spdlog::info("HttpsClientRequest::resolve_hostname - Resolved {} to {}", m_hostname, ip);

    return ip;
}