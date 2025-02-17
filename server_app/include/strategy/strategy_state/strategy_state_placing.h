#ifndef STRATEGY_STATE_PLACING_H
#define STRATEGY_STATE_PLACING_H

#include <strategy/strategy_state/strategy_state.h>

class StrategyStatePlacing : public StrategyState
{
public:
    StrategyStatePlacing(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(double price);

    Order get_buy_spot_order_by_checkpoint(DataModel& checkpoint);
    Order get_sell_perpetual_order_by_checkpoint(DataModel& checkpoint);
};

#endif //STRATEGY_STATE_PLACING_H