#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_market_maker/strategy_market_maker_config.h>
#include <volume_stat/volume_stat.h>
#include <pnl/pnl.h>

class StrategyMarketMakerStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyMarketMakerConfig& m_config;
    const Instrument* m_instrument = nullptr;
    EventBase* m_event_base = nullptr;
    VolumeStat& m_volume_stat;
    PnL& m_pnl;

public:
    StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, const StrategyMarketMakerConfig& config, VolumeStat& volume_stat, PnL& pnl);

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override;

    void on_config_change();

private:
    double m_inventory = 0.0;
    double m_current_price = 0.0;
    double m_last_quoted_price = 0.0;
    double m_min_trade_volume = 0.0;
    double m_volume = 0.0;
    bool   m_is_closing_far_orders = false;
    size_t m_filled_buy_order_count = 0;
    size_t m_filled_sell_order_count = 0;
    std::unordered_map<OrderId, Order> m_open_orders;

    // Generate order
    Order get_limit_order(Order::Side side, double price, double quantity);

    void quote_orders_at_price(double price);
    void start_close_far_orders();
    Task<void> task_close_far_orders();
    Task<void> remove_old_trades();

    void handle_price_update(PriceUpdate price);
    void handle_order_book_snapshot(OrderBookSnapShot* snapshot);
    void handle_order_update(Order& order);
};
