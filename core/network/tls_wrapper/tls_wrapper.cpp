#include "tls_wrapper.h"

TlsWrapper::TlsWrapper(TlsContext* c) : ctx(c)
{
    if (!ctx || !ctx->ctx)
    {
        spdlog::error("TlsWrapper: invalid TlsContext");
        return;
    }

    ssl = SSL_new(ctx->ctx);
    if (!ssl)
    {
        spdlog::error("TlsWrapper: SSL_new failed");
        ERR_print_errors_fp(stderr);
        return;
    }

    // Set mode
    if (ctx->type == TlsType::SERVER)
    {
        SSL_set_accept_state(ssl);
    }
    else
    {
        SSL_set_connect_state(ssl);
    }
}

// Attach file descriptor (non-blocking TCP fd)
bool TlsWrapper::attach_fd(int socket_fd)
{
    fd = socket_fd;
    if (!ssl) return false;

    SSL_set_fd(ssl, fd);
    return true;
}

// Non-blocking handshake
TlsResult TlsWrapper::handshake()
{
    if (handshake_done) return TlsResult::OK;
    if (!ssl) return TlsResult::ERROR;

    int ret = SSL_do_handshake(ssl);
    if (ret == 1)
    {
        handshake_done = true;
        return TlsResult::OK;
    }

    int err = SSL_get_error(ssl, ret);
    switch (err)
    {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            return TlsResult::WANT_IO;

        default:
            spdlog::error("TLS handshake failed: {}", err);
            ERR_print_errors_fp(stderr);
            return TlsResult::ERROR;
    }
}

// TLS read (return <0 = error, 0 = no data/WANT_READ, >0 = bytes read)
int TlsWrapper::read(char* buf, int size)
{
    if (!ssl || !handshake_done) return -1;

    int ret = SSL_read(ssl, buf, size);
    if (ret > 0) return ret;

    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
    {
        return 0; // no data
    }

    spdlog::error("TLS read error: {}", err);
    return -1;
}

// TLS write (return <0 = error, 0 = WANT_WRITE, >0 = bytes written)
int TlsWrapper::write(const char* buf, int size)
{
    if (!ssl || !handshake_done) return -1;

    int ret = SSL_write(ssl, buf, size);
    if (ret > 0) return ret;

    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
    {
        return 0; // try again when epoll reports EPOLLOUT
    }

    spdlog::error("TLS write error: {}", err);
    return -1;
}

void TlsWrapper::shutdown_and_free()
{
    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }

    fd = -1;
    handshake_done = false;
}