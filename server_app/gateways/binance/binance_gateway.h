#pragma once

#include <gateways/gateway.h>
#include <gateways/binance/binance_quoter/binance_quoter_spot.h>
#include <gateways/binance/binance_quoter/binance_quoter_perpetual.h>
#include <gateways/binance/binance_market_data/binance_market_data.h>

class BinanceGateway : public Gateway
{
    // Quoter
    BinanceQuoterSpot m_quoter_spot;
    BinanceQuoterPerpetual m_quoter_perpetual;

    // Market data
    BinanceMarketData m_market_data_spot;
    // BinanceMarketData m_market_data_perpetual;

    // Exchange info
    Json m_symbols_info;

protected:
    virtual ExchangeId get_exchange() override;
    virtual std::string get_name() override;
    virtual Task<std::unordered_set<OrderId>> get_open_orders_on_exchange(std::string symbol) override;
    virtual TaskVoid cancel_all_on_exchange(std::string symbol) override;
    virtual Task<Json> cancel_on_exchange(Order order) override;
    virtual Task<Json> place_on_exchange(Order order) override;

public:
    BinanceGateway(const std::string& key);

    virtual void init() override;
    virtual void subscribe_symbol(std::vector<std::string> symbols) override;
    virtual Task<Json> get_balances() override;
    virtual double round_up_quantity(const std::string& type, const std::string& symbol, double quantity) override;
    virtual size_t get_lot_size(const std::string& type, const std::string& symbol) override;

private:
    Task<Json> get_exchange_info();
    void on_depth_update(const std::string& symbol, Json& payload);

    Json get_spot_symbols_info();
    Json get_perpetual_symbols_info();
    size_t get_rounded_number(const std::string& lot_size);
    std::string round_string_number(const std::string& str_number, size_t precision);

};
