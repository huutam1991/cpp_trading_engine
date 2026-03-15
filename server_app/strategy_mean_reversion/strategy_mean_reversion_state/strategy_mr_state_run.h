#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_mean_reversion/strategy_mean_reversion_config.h>

class StrategyMeanReversionStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyMeanReversionConfig& m_config;

public:
    StrategyMeanReversionStateRun(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config);

    virtual void begin();
    virtual void end();
    virtual Task<void> update(StrategyUpdateData data);
    virtual Json get_info() override;

private:
    double m_current_price = 0.0;
    bool is_taking_profit = false;

    struct OrderInfo
    {
        Order order;
        bool is_handeling = false;
    };

    // Current open orders by price
    std::unordered_map<double, OrderInfo> m_current_open_orders;

    Task<void> handle_price_update(PriceUpdate price);
    Task<void> handle_order_update(Order& order);
};
