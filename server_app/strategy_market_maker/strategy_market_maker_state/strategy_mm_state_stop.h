#pragma once

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyMarketMakerStateStop : public StrategyStateBase
{
public:
    StrategyMarketMakerStateStop();

    virtual void begin() override;
    virtual void end() override;
    virtual TaskVoid update(StrategyUpdateData data) override;

    // virtual Json get_open_orders() override;
};
