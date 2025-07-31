#pragma once

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyPriceArbitrageStateStop : public StrategyStateBase
{
public:
    StrategyPriceArbitrageStateStop();

    virtual void begin() override;
    virtual void end() override;
    virtual TaskVoid update(StrategyUpdateData data) override;

    // virtual JsonNew get_open_orders() override;
};
