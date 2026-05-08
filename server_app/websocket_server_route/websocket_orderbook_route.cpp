#include <json/json.h>

#include "websocket_orderbook_route.h"
#include <order_book/order_book_manager.h>

WebsocketOrderbookRoute::WebsocketOrderbookRoute()
    :   HttpsWebsocketServerRoute(WebsocketRouteName::orderbook, true)
{
    OrderBookManager::instance().register_update([this](OrderBookSnapShotObject snapshot)
    {
        // Skip if no clients connected to this route
        if (m_connected_fds.size() == 0)
        {
            return;
        }

        Json asks;
        for (size_t i = 0; i < snapshot->asks_size; i++)
        {
            asks.push_back({
                {"price", snapshot->asks[i].price},
                {"quantity", snapshot->asks[i].quantity}
            });
        }
        Json bids;
        for (size_t i = 0; i < snapshot->bids_size; i++)
        {
            bids.push_back({
                {"price", snapshot->bids[i].price},
                {"quantity", snapshot->bids[i].quantity}
            });
        }

        Json response = {
            {"route", m_route_name},
            {"instrument", snapshot->instrument->symbol},
            {"bids", bids},
            {"asks", asks}
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