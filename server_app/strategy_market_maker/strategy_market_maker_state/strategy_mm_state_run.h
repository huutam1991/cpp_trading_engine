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

    Json get_info();
    void on_config_change();

private:
    double m_last_quote_price = 0.0;
    double m_inventory = 0.0;
    std::unordered_map<OrderId, Order> m_open_orders;

    enum class Mode { NORMAL, REDUCE, PAUSE };
    Mode m_mode = Mode::NORMAL;

    bool   m_has_last_mid = false;
    double m_last_mid = 0.0;
    double m_r_mean = 0.0;   // EMA mean of returns
    double m_r_var  = 0.0;   // EMA var  of returns

    // Generate order
    Order get_buy_limit_order(double price, double quantity);
    Order get_sell_limit_order(double price, double quantity);
    Order get_market_buy_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity);
    Order get_market_sell_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity);

    void quote_block_orders_at_price(double price);
    void close_far_orders(double price);
    Task<void> task_close_far_orders(double price);

    void handle_price_update(PriceUpdate price);
    void handle_order_book_snapshot(OrderBookSnapShot* snapshot);
    void handle_order_update(Order& order);
};
