#pragma once

#include <functional>
#include <unordered_map>

#include <websocket/websocket_client_async.h>
#include <json/json.h>

#include <instrument/instrument.h>
#include <gateways/binance/binance_market_data/order_book/order_book.h>

class BinanceMarketDataPerpetual
{
public:
    BinanceMarketDataPerpetual(const std::string& url, const std::string& port);
    ~BinanceMarketDataPerpetual();

    void update_url_and_port(const std::string& url, const std::string& port);
    virtual void start();
    void start_websocket(const Instrument* instrument);
    void subscribe_instruments(std::vector<const Instrument*> instruments, std::function<void(const Instrument* symbol, Json& payload)> call_back);

private:
    std::string m_url;
    std::string m_port;
    std::vector<const Instrument*> m_instruments;

    EventBase* m_event_base = nullptr;

    std::unordered_map<const Instrument*, std::shared_ptr<OrderBook>> m_orderbooks;
    std::function<void(const Instrument* symbol, Json& payload)> m_on_callback = nullptr;

    TaskVoid init_order_book();
    TaskVoid check_sync_order_book();
};
