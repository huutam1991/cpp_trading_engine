#include <json/json.h>

#include "websocket_order_route.h"
#include <order_book/order_book_manager.h>

WebsocketOrderRoute::WebsocketOrderRoute() : HttpsWebsocketServerRoute(WebsocketRouteName::order, true)
{
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