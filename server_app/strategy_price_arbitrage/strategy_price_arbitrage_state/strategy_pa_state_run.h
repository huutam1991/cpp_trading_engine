#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyPriceArbitrageStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyPriceArbitrageConfig& m_config;

public:
    StrategyPriceArbitrageStateRun(std::shared_ptr<Gateway> gateway, const StrategyPriceArbitrageConfig& config);

    virtual void begin() override;
    virtual void end() override;
    virtual TaskVoid update(StrategyUpdateData data) override;

    // virtual JsonNew get_open_orders() override;

private:
    Instrument* m_instrument_1 = nullptr;
    Instrument* m_instrument_2 = nullptr;
    Instrument* m_instrument_3 = nullptr;
    double m_symbol_2_price;
    double m_current_price = 0.0;
    bool is_placing_chain_orders = false;

    struct OrderInfo
    {
        Order order;
        bool is_handeling = false;
    };

    // Current open orders by price
    std::unordered_map<double, OrderInfo> m_current_open_orders;

    void on_config_change();

    // Generate order
    Order get_limit_buy_spot_order_by_price(double price);
    Order get_market_buy_spot_order_by_symbol_and_quantity(Instrument* instrument, double quantity);
    Order get_market_sell_spot_order_by_symbol_and_quantity(Instrument* instrument, double quantity);

    void remove_open_order_by_price(double price);
    void check_place_order_at_price(double price);
    void check_cancel_order_at_price(double price);
    void update_orders_at_price(double price);
    TaskVoid handle_price_update(PriceUpdate price);
    TaskVoid handle_order_update(Order& order);
};
