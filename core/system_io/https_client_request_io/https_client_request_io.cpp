#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "https_client_request_io.h"

HttpClientRequestIO::HttpClientRequestIO(const std::string& ip_value, int port_value, TlsWrapper* tls_wrapper)
    : ip{ip_value}, port{port_value}, m_tls_wrapper{tls_wrapper}
{}

int HttpClientRequestIO::generate_fd()
{
    fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) return -1;

    spdlog::info("HttpClientRequestIO::generate_fd - 1");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    spdlog::info("HttpClientRequestIO::generate_fd - 2");

    int res = ::connect(fd, (sockaddr*)&addr, sizeof(addr));
    spdlog::info("HttpClientRequestIO::generate_fd - 3");

    if (res < 0)
    {
        if (errno == EINPROGRESS)
        {
            // Non-blocking connect in progress

    fcntl(fd, F_SETFL, O_NONBLOCK);
    spdlog::info("HttpClientRequestIO::generate_fd - 4");
            return fd;
        }

        close(fd);
        return -1;
    }
    spdlog::info("HttpClientRequestIO::generate_fd - 5");

    return fd;
}

void HttpClientRequestIO::activate()
{
    // TBD
}

int HttpClientRequestIO::handle_io_data()
{
    spdlog::info("HttpClientRequestIO::handle_io_data - 1");
    if (is_connected == false)
    {
    spdlog::info("HttpClientRequestIO::handle_io_data - 2");
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

    spdlog::info("HttpClientRequestIO::handle_io_data - 3");
        if (err != 0)
        {
            spdlog::error("HttpClientRequestIO::handle_io_data - Connect failed: {}, ip: {}, port: {}", strerror(err), ip, port);
            return -1; // close
        }
    spdlog::info("HttpClientRequestIO::handle_io_data - 4");

        is_connected = true;
        spdlog::info("HttpClientRequestIO::handle_io_data - TCP connect success, ip: {}, port: {}", ip, port);

    spdlog::info("HttpClientRequestIO::handle_io_data - 5");
        // Attach fd to TLS wrapper
        if (m_tls_wrapper->attach_fd(fd) == false)
        {
            spdlog::error("HttpClientRequestIO::handle_io_data - attach_fd failed");
            return -1;
        }
    }
    spdlog::info("HttpClientRequestIO::handle_io_data - 6");

    // Check to TLS handshake non-blocking
    if (m_tls_wrapper->is_handshake_done() == false)
    {
    spdlog::info("HttpClientRequestIO::handle_io_data - 7");
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