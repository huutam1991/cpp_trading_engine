#pragma once

#include <functional>
#include <unordered_map>

#include <network/websocket/websocket_client_async.h>
#include <json/json.h>

#include <instrument/instrument.h>
#include <gateways/binance/binance_market_data/order_book/order_book.h>
#include <gateways/binance/binance_market_data/trade_data/trade_data.h>

class BinanceMarketDataPerpetual
{
public:
    BinanceMarketDataPerpetual(const std::string& url, const std::string& port);
    ~BinanceMarketDataPerpetual();

    void update_url_and_port(const std::string& url, const std::string& port);
    virtual void start();
    void start_websocket(const Instrument* instrument);
    void subscribe_instruments(std::vector<const Instrument*> instruments);

private:
    std::string m_url;
    std::string m_port;
    std::vector<const Instrument*> m_instruments;

    EpollBase* m_event_base = nullptr;

    struct MarketData
    {
        std::shared_ptr<OrderBook> orderbook = nullptr;
        std::shared_ptr<BinanceTradeData> trade_data = nullptr;
    };

    std::unordered_map<const Instrument*, std::shared_ptr<MarketData>> m_market_data;

    Task<void> init_order_book();
    Task<void> remove_unsubscribed_instruments();
    Task<void> check_sync_order_book();
};
