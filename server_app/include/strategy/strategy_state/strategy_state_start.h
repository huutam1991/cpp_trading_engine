#ifndef STRATEGY_STATE_START_H
#define STRATEGY_STATE_START_H

#include <strategy/strategy_state/strategy_state.h>

class StrategyStateStart : public StrategyState
{
public:
    StrategyStateStart(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StateData data);
};
#endif //STRATEGY_STATE_START_H