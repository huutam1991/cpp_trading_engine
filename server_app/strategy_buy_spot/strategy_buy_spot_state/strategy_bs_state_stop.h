#pragma once

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyBuySpotStateStop : public StrategyStateBase
{
public:
    StrategyBuySpotStateStop();

    virtual void begin() override;
    virtual void end() override;
    virtual TaskVoid update(StrategyUpdateData data) override;

    // virtual JsonNew get_open_orders() override;
};
