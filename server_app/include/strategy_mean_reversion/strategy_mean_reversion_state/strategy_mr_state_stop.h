#ifndef STRATEGY_MR_STATE_STOP_H
#define STRATEGY_MR_STATE_STOP_H

#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state.h>

class StrategyMeanReversionStateStop : public StrategyMeanReversionState
{
public:
    StrategyMeanReversionStateStop(std::shared_ptr<Gateway>& gateway, StrategyMeanReversionConfig& config);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyMeanReversionData data);

    virtual Json get_open_orders() override;
};

#endif //STRATEGY_MR_STATE_STOP_H