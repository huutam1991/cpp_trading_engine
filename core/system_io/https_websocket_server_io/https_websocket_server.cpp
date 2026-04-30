#include "https_websocket_server.h"
#include "https_websocket_connection.h"

#include <utils/constants.h>

HttpsWebsocketServer::HttpsWebsocketServer(
    int port_value,
    std::function<Task<void>(int)> on_connect_callback,
    std::function<Task<void>(int, std::string)> on_message_callback,
    std::function<Task<void>(int)> on_disconnect_callback)
    : HttpWebsocketServer(port_value, on_connect_callback, on_message_callback, on_disconnect_callback)
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
    connection->set_server_fd(fd);
    establish_connection(connection);

    spdlog::debug("Size of HttpsWebsocketConnectionPool = {}", HttpWebsocketConnectionPool::size());

    return 0;
}