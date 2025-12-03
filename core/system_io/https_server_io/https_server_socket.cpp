#include <openssl/bio.h>
#include <utils/constants.h>

#include "https_server_socket.h"
#include "https_client_socket.h"

#define BACKLOG_SOCKET 125

HttpsServerSocket::HttpsServerSocket(int port_value) : HttpServerSocket{port_value}
{
    server_ctx = new TlsServerContext(SSL_SERVER_CERTIFICATE, SSL_PRIVATE_KEY);
}

int HttpsServerSocket::generate_fd()
{
    // Exactly the same as HttpServerSocket
    return HttpServerSocket::generate_fd();
}

int HttpsServerSocket::activate()
{
    // Nothing to do for server socket
    return 0;
}

int HttpsServerSocket::handle_read()
{
    HttpsClientSocket* client_socket = HttpsClientSocketPool::acquire();
    client_socket->set_server_fd(fd);
    client_socket->set_ssl_context(server_ctx);
    epoll_base->start_living_system_io_object(client_socket);

    spdlog::info("Size of HttpsClientSocketPool = {}", HttpsClientSocketPool::size());

    return 0;
}

int HttpsServerSocket::handle_write()
{
    // Nothing to do for write event
    return 0;
}

void HttpsServerSocket::release()
{
    // Nothing to release for server socket
}