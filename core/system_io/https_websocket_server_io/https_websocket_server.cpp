#include "https_websocket_server.h"
#include "https_websocket_connection.h"

#include <utils/constants.h>

HttpsWebsocketServerIO::HttpsWebsocketServerIO(int port_value) : HttpWebsocketServerIO(port_value)
{
    server_ctx = new TlsServerContext(SSL_SERVER_CERTIFICATE, SSL_PRIVATE_KEY);
}

int HttpsWebsocketServerIO::generate_fd()
{
    return HttpWebsocketServerIO::generate_fd();
}

int HttpsWebsocketServerIO::handle_read()
{
    HttpsWebsocketConnectionIO* connection = HttpsWebsocketConnectionIOPool::acquire();
    connection->set_ssl_context(server_ctx);
    establish_connection(connection);

    spdlog::debug("Size of HttpsWebsocketConnectionIOPool = {}", HttpWebsocketConnectionIOPool::size());

    return 0;
}