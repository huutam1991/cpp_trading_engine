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
    virtual Json get_info() override;

    void on_config_change();

protected:
    virtual void handle_price_update(PriceUpdate& price_update) override;
    virtual void handle_trade_update(TradeUpdate& trade_update) override;
    virtual void handle_order_book_snapshot(OrderBookSnapShot* snapshot) override;
    virtual void handle_order_update(Order& order) override;

private:
    double m_last_price = 0.0;
    double m_price_gap = 0.0;
    double m_inventory = 0.0;

    // Generate order
    Order get_limit_order(Order::Side side, double price, double quantity);
};
