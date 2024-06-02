#ifndef STRATEGY_STATE_PLACING_H
#define STRATEGY_STATE_PLACING_H

#include <strategy/strategy_state/strategy_state.h>

class StrategyStatePlacing : public StrategyState
{
public:
    StrategyStatePlacing(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPoints>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual void run(double price);
};

#endif //STRATEGY_STATE_PLACING_H