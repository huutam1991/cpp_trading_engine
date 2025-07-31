#pragma once

#include <gateways/gateway.h>
#include <gateways/coinbase/coinbase_quoter/coinbase_quoter_spot.h>
#include <gateways/coinbase/coinbase_quoter/coinbase_quoter_perpetual.h>
#include <gateways/coinbase/coinbase_market_data/coinbase_market_data.h>

class CoinbaseGateway : public Gateway
{
    // Quoter
    CoinbaseQuoterSpot m_quoter_spot;
    CoinbaseQuoterPerpetual m_quoter_perpetual;

    // Market data
    CoinbaseMarketData m_market_data_spot;
    // CoinbaseMarketData m_market_data_perpetual;

    // Exchange info
    JsonNew m_symbols_info;

protected:
    virtual ExchangeId get_exchange() override;
    virtual Task<std::unordered_set<OrderId>> get_open_orders_on_exchange(std::string symbol) override;
    virtual TaskVoid cancel_all_on_exchange(std::string symbol) override;
    virtual Task<JsonNew> cancel_on_exchange(Order order) override;
    virtual Task<JsonNew> place_on_exchange(Order order) override;

public:
    CoinbaseGateway(const std::string& key);

    virtual void subscribe_instruments(std::vector<const Instrument*> instruments) override;
    virtual Task<JsonNew> get_balances() override;

private:
    void on_depth_update(const std::string& symbol, JsonNew& payload);

    JsonNew get_spot_symbols_info();
    JsonNew get_perpetual_symbols_info();
    size_t get_rounded_number(const std::string& lot_size);
    std::string round_string_number(const std::string& str_number, size_t precision);

};
