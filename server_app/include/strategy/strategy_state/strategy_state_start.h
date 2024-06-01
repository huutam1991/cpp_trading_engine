#ifndef STRATEGY_STATE_START_H
#define STRATEGY_STATE_START_H

#include <strategy/strategy_state/strategy_state.h>

class StrategyStateStart : public StrategyState
{
public:
    StrategyStateStart(Gateway* gateway, CheckPoints* checkpoints);

    virtual void begin();
    virtual void end();
    virtual void run();
};
#endif //STRATEGY_STATE_START_H