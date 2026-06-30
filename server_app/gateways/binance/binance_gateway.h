#pragma once

#include <coroutine/event_base_manager.h>

#include <gateways/gateway.h>
#include <gateways/binance/binance_account.h>
#include <gateways/binance/binance_quoter/binance_quoter_spot.h>
#include <gateways/binance/binance_quoter/binance_quoter_perpetual.h>
#include <gateways/binance/binance_market_data/binance_market_data_spot.h>
#include <gateways/binance/binance_market_data/binance_market_data_perpetual.h>

class BinanceGateway : public Gateway
{
protected:
    EpollBase* m_epoll_base = nullptr;

    // Market data
    BinanceMarketDataSpot m_market_data_spot;
    BinanceMarketDataPerpetual m_market_data_perpetual;

    std::vector<Instrument> m_instruments;

    virtual std::shared_ptr<OrderEntry> get_order_entry(std::shared_ptr<AccountBase> account) override;
    virtual std::vector<Instrument> fetch_instruments() override;

public:
    BinanceGateway();

    // This constructor is used for child classes that inherit from BinanceGateway
    BinanceGateway(ExchangeId exchange_id);

    virtual void subscribe_instruments(std::vector<const Instrument*> instruments) override;
    virtual void subscribe_instrument(const Instrument* instrument) override;
    virtual void unsubscribe_instrument(const Instrument* instrument) override;

    // Util methods
    virtual std::expected<bool, std::string> validate_account(std::shared_ptr<AccountBase> account) override;
    virtual Json get_status() override;

private:
    Task<Json> get_exchange_info();
    Task<Json> get_exchange_info_perpetual();

    void get_spot_symbols_info();
    void get_perpetual_symbols_info();
    size_t get_rounded_number(const std::string& lot_size);
    std::string round_string_number(const std::string& str_number, size_t precision);

};
