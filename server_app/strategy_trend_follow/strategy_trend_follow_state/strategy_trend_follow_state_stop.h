#pragma once

#include <data_model/savable_object.h>

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <order/order_manager.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyTrendFollowStateStop : public StrategyStateBase
{
public:
    StrategyTrendFollowStateStop();

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override;

    // virtual Json get_open_orders() override;
};
