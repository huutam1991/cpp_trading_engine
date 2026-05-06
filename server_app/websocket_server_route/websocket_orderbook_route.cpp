#include <json/json.h>

#include "websocket_orderbook_route.h"
#include <order_book/order_book_manager.h>

WebsocketOrderbookRoute::WebsocketOrderbookRoute()
    :   HttpsWebsocketServerRoute(WebsocketRouteName::orderbook, true)
{
    OrderBookManager::instance().register_update([this](OrderBookSnapShotObject snapshot)
    {
        Json response = {
            {"route", m_route_name},
            {"instrument", snapshot->instrument->symbol},
            {"best_bid_price", snapshot->get_best_bid()},
            {"best_bid_quantity", snapshot->get_best_bid_quantity()},
            {"best_ask_price", snapshot->get_best_ask()},
            {"best_ask_quantity", snapshot->get_best_ask_quantity()}
        };

        // Broadcast order book update to all connected clients on this route
        for (int fd : m_connected_fds)
        {
            m_server->write_to_connection(fd, response);
        }
    });
}

Task<void> WebsocketOrderbookRoute::on_connect(int fd)
{
    m_connected_fds.insert(fd);
    co_return;
}

Task<void> WebsocketOrderbookRoute::on_message(int fd, std::string message)
{
    // TBD
    spdlog::info("Received message from websocket connection (fd = {}) on route [{}]: {}", fd, m_route_name, message);

    co_return;
}

Task<void> WebsocketOrderbookRoute::on_disconnect(int fd)
{
    m_connected_fds.erase(fd);
    co_return;
}