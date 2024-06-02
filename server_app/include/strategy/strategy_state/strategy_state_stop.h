#ifndef STRATEGY_STATE_STOP_H
#define STRATEGY_STATE_STOP_H

#include <strategy/strategy_state/strategy_state.h>

class StrategyStateStop : public StrategyState
{
public:
    StrategyStateStop(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPoints>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual void run(double price);
};

#endif //STRATEGY_STATE_STOP_H