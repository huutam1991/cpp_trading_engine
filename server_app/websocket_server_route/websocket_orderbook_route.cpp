#include <json/json.h>
#include "websocket_orderbook_route.h"

Task<void> WebsocketOrderbookRoute::on_connect(int fd)
{
    // TBD
    co_return;
}

Task<void> WebsocketOrderbookRoute::on_message(int fd, std::string message)
{
    spdlog::info("Received message from websocket connection (fd = {}) on route [{}]: {}", fd, m_route_name, message);

    Json response = {
        {"message", "This is orderbook route"}
    };

    m_server->write_to_connection(fd, response);

    co_return;
}

Task<void> WebsocketOrderbookRoute::on_disconnect(int fd)
{
    // TBD
    co_return;
}