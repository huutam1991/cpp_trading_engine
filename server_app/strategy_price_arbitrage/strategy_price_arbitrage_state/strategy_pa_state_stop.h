#pragma once

#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state.h>

class StrategyPriceArbitrageStateStop : public StrategyPriceArbitrageState
{
public:
    StrategyPriceArbitrageStateStop(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyPriceArbitrageData data);

    virtual Json get_open_orders() override;
};
