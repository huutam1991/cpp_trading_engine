#include "tls_wrapper.h"


void ssl_info_callback(const SSL* s, int where, int ret)
{
    spdlog::warn("SSL Info Callback triggered");
    spdlog::info("check crash 1");
    const char* str = "";
    int w = where & ~SSL_ST_MASK;
    spdlog::info("check crash 2");

    if (w & SSL_ST_CONNECT) str = "SSL_connect";
    else if (w & SSL_ST_ACCEPT) str = "SSL_accept";
    spdlog::info("check crash 3");

    if (where & SSL_CB_LOOP)
    {
        spdlog::info("check crash 4");
        printf("SSL state (%s): %s\n", str, SSL_state_string_long(s));
    }
    else if (where & SSL_CB_ALERT)
    {
        spdlog::info("check crash 5");
        printf("SSL alert (%s): %s: %s\n",
            (where & SSL_CB_READ) ? "read" : "write",
            SSL_alert_type_string_long(ret),
            SSL_alert_desc_string_long(ret));
    }
    else if (where & SSL_CB_EXIT)
    {
        spdlog::info("check crash 6");
        printf("SSL_exit (%s): ret=%d state=%s\n",
            str, ret, SSL_state_string_long(s));
    }
    spdlog::info("check crash 7");

    spdlog::warn("SSL Info Callback triggered - end");
}

TlsWrapper::TlsWrapper(TlsContext* c) : ctx(c)
{
    if (!ctx || !ctx->ctx)
    {
        spdlog::error("TlsWrapper: invalid TlsContext");
        return;
    }

    ssl = SSL_new(ctx->ctx);

    // SSL_set_info_callback(ssl, ssl_info_callback);
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
            spdlog::error("TlsWrapper::handshake - TLS handshake failed, error code: {}", err);
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

    spdlog::error("TlsWrapper::read - error: {}, connection closed or error occurred", err);
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

    spdlog::error("TlsWrapper::write - error: {}", err);
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