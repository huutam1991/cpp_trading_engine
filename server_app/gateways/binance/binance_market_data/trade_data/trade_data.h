#pragma once

#include <network/https_client_websocket/https_client_websocket.h>

#include <instrument/instrument.h>

class BinanceTradeData
{
    std::string m_symbol;
    EpollBase* m_event_base;
    const Instrument* m_instrument = nullptr;
    std::shared_ptr<HttpsClientWebsocket> m_websocket = nullptr;

    void start();

public:
    BinanceTradeData(const std::string& symbol, EpollBase* event_base);
    ~BinanceTradeData() {}
};