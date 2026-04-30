#include "https_websocket_connection.h"

int HttpsWebsocketConnectionIO::activate()
{
    if (!tls_wrapper || !tls_wrapper->attach_fd(fd))
    {
        spdlog::error("HttpsWebsocketConnectionIO::activate - attach_fd failed");
        return -1;
    }

    current_state = State::CONNECTING_AND_HANDSHAKING;
    return 0;
}

int HttpsWebsocketConnectionIO::handle_read()
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
    return HttpWebsocketConnectionIO::handle_read();
}

int HttpsWebsocketConnectionIO::handle_write()
{
    if (!tls_wrapper->is_handshake_done())
    {
        TlsResult result = tls_wrapper->handshake();
        return result != TlsResult::ERROR ? 0 : -1;
    }

    return check_to_write();
}

int HttpsWebsocketConnectionIO::read_buffer(char* const buffer, std::size_t size)
{
    if (!tls_wrapper)
    {
        return -1;
    }

    return tls_wrapper->read(buffer, size);
}

int HttpsWebsocketConnectionIO::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    if (!tls_wrapper)
    {
        return -1;
    }

    return tls_wrapper->write(buffer, 0, size);
}

void HttpsWebsocketConnectionIO::release()
{
    if (tls_wrapper)
    {
        delete tls_wrapper;
        tls_wrapper = nullptr;
    }

    if (on_disconnect != nullptr)
    {
        on_disconnect(fd).start_running_on(epoll_base);
    }

    HttpsWebsocketConnectionIOPool::release(this);
}