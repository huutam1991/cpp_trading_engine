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

    Order get_close_buy_spot_order_by_checkpoint(DataModel& checkpoint);
    Order get_close_sell_perpetual_order_by_checkpoint(DataModel& checkpoint);
};

#endif //STRATEGY_STATE_STOP_H