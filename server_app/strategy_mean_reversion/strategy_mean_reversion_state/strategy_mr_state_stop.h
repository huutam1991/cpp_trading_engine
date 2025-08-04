#pragma once

#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state.h>

class StrategyMeanReversionStateStop : public StrategyMeanReversionState
{
public:
    StrategyMeanReversionStateStop(std::shared_ptr<Gateway>& gateway, StrategyMeanReversionConfig& config);

    virtual void begin();
    virtual void end();
    virtual Task<void> run(StrategyMeanReversionData data);

    virtual Json get_open_orders() override;
};
