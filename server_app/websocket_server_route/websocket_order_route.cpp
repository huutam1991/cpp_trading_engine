#include <json/json.h>

#include "websocket_order_route.h"
#include <order/order_manager.h>

WebsocketOrderRoute::WebsocketOrderRoute() : HttpsWebsocketServerRoute(WebsocketRouteName::order, true)
{
    OrderManager::instance().register_order_update([this](Order order)
    {
        Json order_json = order.to_json();

        for (int fd : m_connected_fds)
        {
            m_server->write_to_connection(fd, order_json);
        }
    });
}

Task<void> WebsocketOrderRoute::on_connect(int fd)
{
    m_connected_fds.insert(fd);
    co_return;
}

Task<void> WebsocketOrderRoute::on_message(int fd, std::string message)
{
    co_return;
}

Task<void> WebsocketOrderRoute::on_disconnect(int fd)
{
    m_connected_fds.erase(fd);
    co_return;
}