#pragma once

#include <strategy_buy_spot/strategy_buy_spot_state/strategy_state.h>

class StrategyStateCloseAllPositions : public StrategyStateFirst
{
public:
    StrategyStateCloseAllPositions(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);
};
