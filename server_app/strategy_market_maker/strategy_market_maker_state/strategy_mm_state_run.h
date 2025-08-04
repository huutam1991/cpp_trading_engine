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

public:
    StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, const StrategyMarketMakerConfig& config);

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;

    // virtual Json get_open_orders() override;
    void on_config_change();

private:
    size_t m_lot_size = 1;
    double m_current_price = 0.0;
    bool m_is_placing = false;

    // Current orders
    Order m_current_order_buy;
    Order m_current_order_sell;

    // Helper methods
    void update_lot_size();
    double local_round_up_quantity(double quantity);

    // Generate order
    Order get_limit_buy_spot_order(double price, double quantity);
    Order get_limit_sell_spot_order(double price, double quantity);
    Order get_market_buy_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity);
    Order get_market_sell_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity);

    Task<void> handle_price_update(PriceUpdate price);
    Task<void> handle_order_update(Order& order);
};
