#include "https_websocket_server.h"
#include "https_websocket_connection.h"

#include <utils/constants.h>

HttpsWebsocketServer::HttpsWebsocketServer(int port_value) : HttpWebsocketServer(port_value)
{
    server_ctx = new TlsServerContext(SSL_SERVER_CERTIFICATE, SSL_PRIVATE_KEY);
}

int HttpsWebsocketServer::generate_fd()
{
    return HttpWebsocketServer::generate_fd();
}

int HttpsWebsocketServer::handle_read()
{
    HttpsWebsocketConnection* connection = HttpsWebsocketConnectionPool::acquire();
    connection->set_ssl_context(server_ctx);
    establish_connection(connection);

    spdlog::debug("Size of HttpsWebsocketConnectionPool = {}", HttpWebsocketConnectionPool::size());

    return 0;
}