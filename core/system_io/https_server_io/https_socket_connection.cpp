#include "https_socket_connection.h"

#define BUFFER_SIZE 2048

void HttpsSocketConnection::set_ssl_context(TlsContext* tls_context)
{
    tls_wrapper = new TlsWrapper(tls_context);
}

void HttpsSocketConnection::refresh()
{
    fd = -1;
}

int HttpsSocketConnection::generate_fd()
{
    fd = HttpSocketConnection::generate_fd();
    return fd;
}

int HttpsSocketConnection::activate()
{
    if (tls_wrapper->attach_fd(fd) == false)
    {
        spdlog::error("HttpsSocketConnection::generate_fd - attach_fd failed");
        return -1;
    }

    return 0;
}

int HttpsSocketConnection::handle_read()
{
    if (tls_wrapper == nullptr)
    {
        spdlog::error("HttpsSocketConnection::handle_read - tls_wrapper is null, socket fd = {}", fd);
        return -1;
    }

    // Continue with ssl_accept if it's not finish yet
    if (tls_wrapper->is_handshake_done() == false)
    {
        TlsResult result = tls_wrapper->handshake();
        return result != TlsResult::ERROR ? 0 : -1;
    }
    // If ssl_accept is finished, handle client request
    else
    {
        return HttpSocketConnection::handle_read();
    }
}

int HttpsSocketConnection::handle_write()
{
    if (tls_wrapper == nullptr)
    {
        spdlog::error("HttpsSocketConnection::handle_write - tls_wrapper is null, socket fd = {}", fd);
        return -1;
    }

    // Continue with ssl_accept if it's not finish yet
    if (tls_wrapper->is_handshake_done() == false)
    {
        TlsResult result = tls_wrapper->handshake();
        return result != TlsResult::ERROR ? 0 : -1;
    }

    return 0;
}

void HttpsSocketConnection::release()
{
    if (tls_wrapper)
    {
        delete tls_wrapper;
        tls_wrapper = nullptr;
    }

    HttpsSocketConnectionPool::release(this);
}

int HttpsSocketConnection::read_buffer(char* const buffer)
{
    if (tls_wrapper == nullptr)
    {
        spdlog::error("HttpsSocketConnection::read_buffer - tls_wrapper is null, socket fd = {}", fd);
        return -1;
    }

    return tls_wrapper->read(buffer, BUFFER_SIZE);
}

int HttpsSocketConnection::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    if (tls_wrapper == nullptr)
    {
        spdlog::error("HttpsSocketConnection::write_to_socket_io - tls_wrapper is null, socket fd = {}", fd);
        return -1;
    }

    return tls_wrapper->write(buffer, 0, size);
}