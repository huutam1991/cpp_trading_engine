#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_trend_follow/strategy_trend_follow_config.h>

class StrategyTrendFollowStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyTrendFollowConfig& m_config;
    const Instrument* m_instrument = nullptr;
    EventBase* m_event_base = nullptr;

public:
    StrategyTrendFollowStateRun(std::shared_ptr<Gateway> gateway, const StrategyTrendFollowConfig& config);

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override;

    void on_config_change();

private:
    std::unordered_map<OrderId, Order> m_open_bid_orders;
    std::unordered_map<OrderId, Order> m_open_ask_orders;

    // Generate order
    Order get_buy_limit_order(double price, double quantity);
    Order get_sell_limit_order(double price, double quantity);

    void handle_price_update(PriceUpdate price);
    void handle_order_book_snapshot(OrderBookSnapShot* snapshot);
    void handle_order_update(Order& order);
};
