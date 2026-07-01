#pragma once

#include <coroutine/event_base_manager.h>

#include <gateways/order_entry.h>
#include <gateways/binance/binance_account.h>
#include <gateways/binance/binance_quoter/binance_quoter_spot.h>
#include <gateways/binance/binance_quoter/binance_quoter_perpetual.h>

class BinanceOrderEntry : public OrderEntry
{
    // BinanceAccount m_account;

    // Quoter
    BinanceQuoterSpot m_quoter_spot;
    BinanceQuoterPerpetual m_quoter_perpetual;

protected:
    virtual Task<std::unordered_set<OrderId>> get_open_orders_on_exchange(std::string symbol) override;
    virtual Task<void> cancel_all_on_exchange(std::string symbol) override;
    virtual Task<Json> cancel_on_exchange(Order order) override;
    virtual Task<Json> place_on_exchange(Order order) override;

public:
    BinanceOrderEntry(std::shared_ptr<AccountBase> account, EventBase* event_base);

    // This constructor is used for child classes that inherit from BinanceOrderEntry
    BinanceOrderEntry(ExchangeId exchange_id, std::shared_ptr<AccountBase> account, EventBase* event_base);

    // Util methods
    virtual Task<Json> get_balances() override;
    virtual Task<Json> get_positions() override;

private:
    size_t get_rounded_number(const std::string& lot_size);
    std::string round_string_number(const std::string& str_number, size_t precision);

};
