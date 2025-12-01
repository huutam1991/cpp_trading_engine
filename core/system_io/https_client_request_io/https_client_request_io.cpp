#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "https_client_request_io.h"

HttpClientRequestIO::HttpClientRequestIO(const std::string& ip_value, int port_value, TlsWrapper* tls_wrapper)
    : ip{ip_value}, port{port_value}, m_tls_wrapper{tls_wrapper}
{}

int HttpClientRequestIO::generate_fd()
{
    fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int res = ::connect(fd, (sockaddr*)&addr, sizeof(addr));

    if (res < 0)
    {
        if (errno == EINPROGRESS)
        {
            // Non-blocking connect in progress
            return fd;
        }

        close(fd);
        return -1;
    }

    return fd;
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
        m_tls_wrapper->handshake();
        return 0;
    }
    else
    {
    }

    return 0;
}

void HttpClientRequestIO::release()
{
}