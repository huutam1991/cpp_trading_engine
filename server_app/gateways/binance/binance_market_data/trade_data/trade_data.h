#pragma once

#include <websocket/websocket_client_async.h>
#include <ioc_pool.h>

#include <instrument/instrument.h>

class BinanceTradeData
{
    std::string m_symbol;
    net::io_context& m_ioc;
    EventBase* m_event_base;
    const Instrument* m_instrument = nullptr;
    std::shared_ptr<WebsocketClientAsync> m_websocket;

    void start();

public:
    BinanceTradeData(const std::string& symbol, net::io_context& ioc, EventBase* event_base);
    ~BinanceTradeData()
    {
        m_websocket->close();
    }
};