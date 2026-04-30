#include "websocket_server_routes.h"
#include "websocket_orderbook_route.h"

WebsocketServerRoutes::WebsocketServerRoutes(int port, EpollBase* epoll_base)
    :   m_server(std::make_unique<HttpsWebsocketServer>(port, epoll_base))
{
    // Default route
    m_server->add_route(std::make_unique<HttpsWebsocketServerRoute>());

    // Orderbook
    m_server->add_route(std::make_unique<WebsocketOrderbookRoute>());
}