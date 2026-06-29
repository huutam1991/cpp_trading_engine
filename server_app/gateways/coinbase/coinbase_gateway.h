#pragma once

#include <gateways/gateway.h>
#include <gateways/coinbase/coinbase_account.h>
#include <gateways/coinbase/coinbase_quoter/coinbase_quoter_spot.h>
#include <gateways/coinbase/coinbase_quoter/coinbase_quoter_perpetual.h>
#include <gateways/coinbase/coinbase_market_data/coinbase_market_data.h>

class CoinbaseGateway : public Gateway
{
    CoinbaseAccount m_account;

    // Quoter
    CoinbaseQuoterSpot m_quoter_spot;
    CoinbaseQuoterPerpetual m_quoter_perpetual;

    // Market data
    CoinbaseMarketData m_market_data_spot;
    // CoinbaseMarketData m_market_data_perpetual;

    // Exchange info
    Json m_symbols_info;

protected:
    virtual ExchangeId get_exchange() override;
    virtual std::shared_ptr<OrderEntry> get_order_entry() override;

public:
    CoinbaseGateway();

    virtual void subscribe_instruments(std::vector<const Instrument*> instruments) override;
    virtual void subscribe_instrument(const Instrument* instrument) override {}
    virtual void unsubscribe_instrument(const Instrument* instrument) override {}

    // Util methods
    virtual std::expected<bool, std::string> validate_account(std::shared_ptr<AccountBase> account) override;
    virtual Json get_status() override;

private:
    void on_depth_update(const std::string& symbol, Json& payload);

    Json get_spot_symbols_info();
    Json get_perpetual_symbols_info();
    size_t get_rounded_number(const std::string& lot_size);
    std::string round_string_number(const std::string& str_number, size_t precision);

};
