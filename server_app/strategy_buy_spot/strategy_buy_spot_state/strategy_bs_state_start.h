#pragma once

#include <strategy/strategy_state/strategy_state.h>

class StrategyBuySpotStateStart : public StrategyStateFirst
{
public:
    StrategyBuySpotStateStart(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);
};
