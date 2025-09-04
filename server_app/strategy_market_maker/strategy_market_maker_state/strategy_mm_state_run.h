#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_market_maker/strategy_market_maker_config.h>

class StrategyMarketMakerStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyMarketMakerConfig& m_config;
    const Instrument* m_instrument = nullptr;
    EventBase* m_event_base = nullptr;

public:
    StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, const StrategyMarketMakerConfig& config);

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override;

    void on_config_change();

private:
    double m_inventory = 0.0;
    double m_current_price = 0.0;
    bool   m_place_initial_orders = false;
    bool   m_is_closing_far_orders = false;
    std::unordered_map<OrderId, Order> m_open_orders;

    // Generate order
    Order get_limit_order(Order::Side side, double price, double quantity);

    void quote_orders_at_price(double price);
    void start_close_far_orders();
    Task<void> task_close_far_orders();

    void handle_price_update(PriceUpdate price);
    void handle_order_book_snapshot(OrderBookSnapShot* snapshot);
    void handle_order_update(Order& order);
};
