#ifndef STRATEGY_PA_STATE_RUN_H
#define STRATEGY_PA_STATE_RUN_H

#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state.h>

class StrategyPriceArbitrageStateRun : public StrategyPriceArbitrageState
{
public:
    StrategyPriceArbitrageStateRun(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);
};

#endif //STRATEGY_PA_STATE_RUN_H