#pragma once

#include <strategy/strategy_state/strategy_state.h>

class StrategyStatePlacing : public StrategyStateFirst
{
public:
    StrategyStatePlacing(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);

    Order get_buy_spot_order_by_checkpoint(DataModel& checkpoint);
    Order get_sell_perpetual_order_by_checkpoint(DataModel& checkpoint);
};
