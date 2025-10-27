#include <openssl/bio.h>
#include <utils/constants.h>

#include "https_server_socket.h"
#include "https_client_socket.h"

#define BACKLOG_SOCKET 125

HttpsServerSocket::HttpsServerSocket(int port_value) : HttpServerSocket{port_value}
{
    ctx = create_context();
    configure_context(ctx);
}

SSL_CTX* HttpsServerSocket::create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        spdlog::error("Unable to create SSL context");
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void HttpsServerSocket::configure_context(SSL_CTX *ctx)
{
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, SSL_SERVER_CERTIFICATE, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, SSL_PRIVATE_KEY, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void HttpsServerSocket::generate_fd()
{
    // Exactly the same as HttpServerSocket
    HttpServerSocket::generate_fd();
}

int HttpsServerSocket::handle_io_data()
{
    HttpsClientSocket* client_socket = HttpsClientSocketPool::acquire();
    client_socket->set_server_fd(fd);
    epoll_base->start_living_on(client_socket);

    spdlog::info("Size of HttpsClientSocketPool = {}", HttpsClientSocketPool::size());

    return 0;
}

void HttpsServerSocket::release()
{
    // Nothing to release for server socket
}