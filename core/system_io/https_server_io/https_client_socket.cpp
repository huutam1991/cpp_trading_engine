#include "https_client_socket.h"

#define BUFFER_SIZE 2048

void HttpsClientSocket::set_ssl_context(TlsContext* tls_context)
{
    tls_wrapper = new TlsWrapper(tls_context);
}

int HttpsClientSocket::generate_fd()
{
    fd = HttpClientSocket::generate_fd();
    return fd;
}

int HttpsClientSocket::activate()
{
    if (tls_wrapper->attach_fd(fd) == false)
    {
        spdlog::error("HttpsClientSocket::generate_fd - attach_fd failed");
        return -1;
    }

    return 0;
}

int HttpsClientSocket::handle_read()
{
    // Continue with ssl_accept if it's not finish yet
    if (tls_wrapper->is_handshake_done() == false)
    {
        TlsResult result = tls_wrapper->handshake();
        return result != TlsResult::ERROR ? 0 : -1;
    }
    // If ssl_accept is finished, handle client request
    else
    {
        return HttpClientSocket::handle_read();
    }
}

int HttpsClientSocket::handle_write()
{
    // Continue with ssl_accept if it's not finish yet
    if (tls_wrapper->is_handshake_done() == false)
    {
        TlsResult result = tls_wrapper->handshake();
        return result != TlsResult::ERROR ? 0 : -1;
    }

    return 0;
}

void HttpsClientSocket::release()
{
    if (tls_wrapper)
    {
        delete tls_wrapper;
        tls_wrapper = nullptr;
    }

    HttpsClientSocketPool::release(this);
}

int HttpsClientSocket::read_buffer(char* const buffer)
{
    if (tls_wrapper == nullptr)
    {
        spdlog::error("HttpsClientSocket::read_buffer - tls_wrapper is null");
        return -1;
    }

    return tls_wrapper->read(buffer, BUFFER_SIZE);
}

void HttpsClientSocket::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    if (tls_wrapper == nullptr)
    {
        spdlog::error("HttpsClientSocket::write_to_socket_io - tls_wrapper is null");
        return;
    }

    tls_wrapper->write(buffer, size);
}