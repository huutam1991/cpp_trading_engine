#pragma once

#include <coroutine/event_base_manager.h>

#include <gateways/gateway.h>
#include <gateways/binance/binance_quoter/binance_quoter_spot.h>
#include <gateways/binance/binance_quoter/binance_quoter_perpetual.h>
#include <gateways/binance/binance_market_data/binance_market_data_spot.h>
#include <gateways/binance/binance_market_data/binance_market_data_perpetual.h>

class BinanceGateway : public Gateway
{
    // Quoter
    BinanceQuoterSpot m_quoter_spot;
    BinanceQuoterPerpetual m_quoter_perpetual;

    // Market data
    BinanceMarketDataSpot m_market_data_spot;
    BinanceMarketDataPerpetual m_market_data_perpetual;

    std::vector<Instrument> m_instruments;

    EpollBase* m_epoll_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY);

protected:
    virtual ExchangeId get_exchange() override;
    virtual std::vector<Instrument> fetch_instruments() override;
    virtual Task<std::unordered_set<OrderId>> get_open_orders_on_exchange(std::string symbol) override;
    virtual Task<void> cancel_all_on_exchange(std::string symbol) override;
    virtual Task<Json> cancel_on_exchange(Order order) override;
    virtual Task<Json> place_on_exchange(Order order) override;

public:
    BinanceGateway(const std::string& key);

    virtual void subscribe_instruments(std::vector<const Instrument*> instruments) override;
    virtual void subscribe_instrument(const Instrument* instrument) override;
    virtual void unsubscribe_instrument(const Instrument* instrument) override;
    virtual Task<Json> get_balances() override;

private:
    Task<Json> get_exchange_info();
    Task<Json> get_exchange_info_perpetual();

    void get_spot_symbols_info();
    void get_perpetual_symbols_info();
    size_t get_rounded_number(const std::string& lot_size);
    std::string round_string_number(const std::string& str_number, size_t precision);

};
