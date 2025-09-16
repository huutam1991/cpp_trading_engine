#pragma once

#include <data_model/savable_object.h>

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <order/order_manager.h>

class StrategyMarketMakerStateStop : public StrategyStateBase
{

public:
    StrategyMarketMakerStateStop();

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override;

    // virtual Json get_open_orders() override;
};
