#pragma once

#include <functional>
#include <unordered_map>

#include <json/json.h>

#include <instrument/instrument.h>
#include <gateways/binance/binance_market_data/order_book/binance_order_book.h>
#include <gateways/binance/binance_market_data/trade_data/trade_data.h>

class BinanceMarketDataPerpetual
{
public:
    BinanceMarketDataPerpetual(const std::string& url, const std::string& port);
    ~BinanceMarketDataPerpetual();

    void update_url_and_port(const std::string& url, const std::string& port);
    virtual void start(const Instrument* instrument);
    void start_websocket(const Instrument* instrument);
    void subscribe_instruments(std::vector<const Instrument*> instruments);
    void subscribe_instrument(const Instrument* instrument);
    void unsubscribe_instrument(const Instrument* instrument)
    {
        // m_instruments.erase(std::remove(m_instruments.begin(), m_instruments.end(), instrument), m_instruments.end());
    }

private:
    std::string m_url;
    std::string m_port;

    EpollBase* m_event_base = nullptr;

    struct MarketData
    {
        std::shared_ptr<BinanceOrderBook> orderbook = nullptr;
        std::shared_ptr<BinanceTradeData> trade_data = nullptr;
    };

    std::unordered_map<const Instrument*, std::shared_ptr<MarketData>> m_market_data;
    bool m_start_sync_order_book = false;

    Task<void> init_order_book(const Instrument* instrument);
    Task<void> remove_unsubscribed_instruments();
    Task<void> check_sync_order_book(const Instrument* instrument);
};
