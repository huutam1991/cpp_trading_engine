#pragma once

#include <memory>
#include <string>
#include <functional>

#include <websocket/websocket_client_async.h>
#include <ioc_pool.h>

class OrderBookWebsocket
{
public:
    OrderBookWebsocket(const std::string& symbol, size_t depth_level, net::io_context& ioc, EventBase* event_base, std::function<void(std::string)> on_order_book_ws);

private:
    std::string m_symbol;
    size_t m_depth_level;
    net::io_context& m_ioc;
    EventBase* m_event_base;
    std::function<void(std::string)> m_on_order_book_ws;

    std::shared_ptr<WebsocketClientAsync> m_websocket;

    TaskVoid keep_websocket_alive();
    
};