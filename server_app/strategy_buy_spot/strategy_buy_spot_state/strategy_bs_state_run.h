#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_buy_spot/strategy_buy_spot_config.h>

class StrategyBuySpotStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyBuySpotConfig& m_config;

public:
    StrategyBuySpotStateRun(std::shared_ptr<Gateway> gateway, const StrategyBuySpotConfig& config);

    virtual void begin() override;
    virtual void end() override;
    virtual TaskVoid update(StrategyUpdateData data) override;

    // virtual Json get_open_orders() override;

private:
    Instrument* m_instrument = nullptr;
    double m_current_price = 0.0;

    // Current open orders by order id
    std::unordered_map<OrderId, Order> m_current_open_orders;

    void on_config_change();

    // Generate order
    Order get_limit_buy_spot_order_by_price(double price);

    void remove_open_order_by_price(double price);
    void check_place_order_at_price(double price);
    void check_cancel_order_at_price(double price);
    void update_orders_at_price(double price);
    TaskVoid handle_price_update(PriceUpdate price);
    TaskVoid handle_order_update(Order& order);
};
