#ifndef BINANCE_GATEWAY_H
#define BINANCE_GATEWAY_H

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
    BinanceMarketData m_market_data_perpetual;

    // Exchange info
    Json m_symbols_info;

public:
    BinanceGateway(const std::string& key);

    virtual void subscribe_symbol(const std::string& symbol);
    virtual Json place(Order order) override;
    virtual Json get_balances();
    virtual double round_up_quantity(const std::string& type, const std::string& symbol, double quantity);

private:
    void on_depth_update(const std::string& symbol, Json& payload);

    Json get_spot_symbols_info();
    Json get_perpetual_symbols_info();
    size_t get_rounded_number(const std::string& lot_size);
    std::string round_string_number(const std::string& str_number, size_t precision);

};

#endif //BINANCE_GATEWAY_H