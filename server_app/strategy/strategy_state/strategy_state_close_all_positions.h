#pragma once

#include <strategy/strategy_state/strategy_state.h>

class StrategyStateCloseAllPositions : public StrategyState
{
public:
    StrategyStateCloseAllPositions(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);
};
