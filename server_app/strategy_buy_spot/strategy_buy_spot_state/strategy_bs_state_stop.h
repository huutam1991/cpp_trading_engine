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
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override { return {}; }

    // virtual Json get_open_orders() override;
};
