#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "https_client_request_io.h"

HttpClientRequestIO::HttpClientRequestIO(const std::string& hostname_value, int port_value)
    : hostname{hostname_value}, ip{resolve_hostname()}, port{port_value}, m_tls_wrapper{std::make_unique<TlsWrapper>(get_tls_context())}
{
    // Set SNI
    SSL_set_tlsext_host_name(m_tls_wrapper->get_ssl(), hostname.c_str());
}

TlsContext* HttpClientRequestIO::get_tls_context()
{
    static TlsClientContext client_ctx{false, ""};
    return &client_ctx;
}

std::string HttpClientRequestIO::resolve_hostname()
{
    addrinfo hints{};
    hints.ai_family   = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    addrinfo* result = nullptr;
    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (ret != 0)
    {
        spdlog::error("HttpClientRequestIO::resolve_hostname - getaddrinfo failed for {}: {}", hostname, gai_strerror(ret));
        return ""; // fail
    }

    char ip_str[INET_ADDRSTRLEN] = {0};

    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, sizeof(ip_str));
    std::string ip = ip_str;

    freeaddrinfo(result);

    spdlog::info("HttpClientRequestIO::resolve_hostname - Resolved {} to {}", hostname, ip);

    return ip;
}

int HttpClientRequestIO::generate_fd()
{
    fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) return -1;

    return fd;
}

int HttpClientRequestIO::activate()
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int res = ::connect(fd, (sockaddr*)&addr, sizeof(addr));

    if (res < 0)
    {
        // Non-blocking connect in progress
        if (errno == EINPROGRESS)
        {
            return 0;
        }

        close(fd);
        return -1;
    }
    return 0;
}

int HttpClientRequestIO::handle_io_data()
{
    if (is_connected == false)
    {
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

        if (err != 0)
        {
            spdlog::error("HttpClientRequestIO::handle_io_data - Connect failed: {}, ip: {}, port: {}", strerror(err), ip, port);
            return -1; // close
        }

        is_connected = true;
        spdlog::info("HttpClientRequestIO::handle_io_data - TCP connect success, ip: {}, port: {}", ip, port);

        // Attach fd to TLS wrapper
        if (m_tls_wrapper->attach_fd(fd) == false)
        {
            spdlog::error("HttpClientRequestIO::handle_io_data - attach_fd failed");
            return -1;
        }
    }

    // Check to TLS handshake non-blocking
    if (m_tls_wrapper->is_handshake_done() == false)
    {
        TlsResult result = m_tls_wrapper->handshake();
        return result != TlsResult::ERROR ? 0 : -1;
    }
    else
    {
    }

    return 0;
}

void HttpClientRequestIO::release()
{
    spdlog::info("HttpClientRequestIO::release - Releasing HttpClientRequestIO, fd = {}, ip: {}, port: {}", fd, ip, port);
    fd = -1;
    is_connected = false;
}