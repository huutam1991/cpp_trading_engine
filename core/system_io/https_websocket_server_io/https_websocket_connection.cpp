#include "https_websocket_connection.h"

int HttpsWebsocketConnection::activate()
{
    if (!tls_wrapper || !tls_wrapper->attach_fd(fd))
    {
        spdlog::error("HttpsWebsocketConnection::activate - attach_fd failed");
        return -1;
    }

    current_state = State::CONNECTING_AND_HANDSHAKING;
    return 0;
}

int HttpsWebsocketConnection::handle_read()
{
    // TLS handshake first
    if (!tls_wrapper->is_handshake_done())
    {
        TlsResult result = tls_wrapper->handshake();

        if (result == TlsResult::ERROR)
        {
            return -1;
        }

        if (tls_wrapper->is_handshake_done())
        {
            current_state = State::READING_AND_WRITING;
        }

        return 0;
    }

    // After TLS handshake → behave like normal websocket
    return HttpWebsocketConnection::handle_read();
}

int HttpsWebsocketConnection::handle_write()
{
    if (!tls_wrapper->is_handshake_done())
    {
        TlsResult result = tls_wrapper->handshake();
        return result != TlsResult::ERROR ? 0 : -1;
    }

    return check_to_write();
}

int HttpsWebsocketConnection::read_buffer(char* const buffer, std::size_t size)
{
    if (!tls_wrapper)
    {
        return -1;
    }

    return tls_wrapper->read(buffer, size);
}

int HttpsWebsocketConnection::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    if (!tls_wrapper)
    {
        return -1;
    }

    return tls_wrapper->write(buffer, 0, size);
}

void HttpsWebsocketConnection::release()
{
    if (tls_wrapper)
    {
        delete tls_wrapper;
        tls_wrapper = nullptr;
    }

    run_on_disconnect().start_running_on((EventBase*)epoll_base);

    HttpsWebsocketConnectionPool::release(this);
}