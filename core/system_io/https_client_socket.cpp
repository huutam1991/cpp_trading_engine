#include "https_client_socket.h"

#define BUFFER_SIZE 2048

void HttpsClientSocket::clear()
{
    // Clean up SSL resources
    SSL_shutdown(ssl);
    SSL_free(ssl);

    server_fd = -1;
    ctx = nullptr;
    ssl = nullptr;
    ssl_accept_success = false;
    save_buffer = "";
}

void HttpsClientSocket::set_ssl_context(SSL_CTX* ctx_value)
{
    ctx = ctx_value;
}

int HttpsClientSocket::generate_fd()
{
    fd = HttpClientSocket::generate_fd();

    if (fd < 0)
    {
        return fd;
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);

    ssl_accept_success = false;

    // Do ssl_accept
    SSL_ACCEPT_STATUS accept_status = do_ssl_accept();
    if (accept_status == SSL_ACCEPT_STATUS::ERROR)
    {
        return -1;
    }

    return fd;
}

SSL_ACCEPT_STATUS HttpsClientSocket::do_ssl_accept()
{
    SSL_ACCEPT_STATUS accept_status;

    try
    {
        int accept = SSL_accept(ssl);

        switch (SSL_get_error(ssl, accept))
        {
            case SSL_ERROR_NONE:
                accept_status = SSL_ACCEPT_STATUS::OK;
                break;
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
                accept_status = SSL_ACCEPT_STATUS::WANT_IO;
                break;
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_SYSCALL:
            default:
                accept_status = SSL_ACCEPT_STATUS::ERROR;
                break;
        }
    }
    catch(const std::exception& e)
    {
        // LOG(ERROR) << "HttpsServer - SSL_accept error = " << e.what() << '\n';

        // Close socket fd
        // LOG(INFO) << "error https - accept_new_connection: " << std::strerror(errno) << std::endl;
        // LOG(INFO) << "close socket id: " << client_fd << std::endl;
        return SSL_ACCEPT_STATUS::ERROR;
    }

    if (accept_status == SSL_ACCEPT_STATUS::ERROR)
    {
        // LOG(INFO) << "error https - accept_new_connection: " << std::strerror(errno) << std::endl;
        // LOG(INFO) << "close socket id: " << client_fd << std::endl;
        return SSL_ACCEPT_STATUS::ERROR;
    }

    if (accept_status == SSL_ACCEPT_STATUS::OK)
    {
        ssl_accept_success = true;

        // Set non-blocking BIO
        BIO *ssl_bio = BIO_new_socket(fd, BIO_NOCLOSE);
        BIO_set_write_buf_size(ssl_bio, 15 * 1024 * 1024);
        SSL_set_bio(ssl, ssl_bio, ssl_bio);
        BIO_set_nbio(ssl_bio, 1);
    }

    return accept_status;
}

int HttpsClientSocket::handle_io_data()
{
    // Continue with ssl_accept if it's not finish yet
    if (ssl_accept_success == false)
    {
        return (int)do_ssl_accept();
    }
    // If ssl_accept is finished, handle client request
    else
    {
        return HttpClientSocket::handle_io_data();
    }
}

void HttpsClientSocket::release()
{
    HttpsClientSocketPool::release(this);
    close(fd);
}

int HttpsClientSocket::read_buffer(char* const buffer)
{
    return SSL_read(ssl, buffer, BUFFER_SIZE);
}

void HttpsClientSocket::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    SSL_write(ssl, buffer, size);
}