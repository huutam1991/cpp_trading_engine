#ifndef STRATEGY_STATE_MONITORING_H
#define STRATEGY_STATE_MONITORING_H

#include <strategy/strategy_state/strategy_state.h>

class StrategyStateMonitoring : public StrategyState
{
public:
    StrategyStateMonitoring(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);

private:
    std::unordered_map<OrderId, DataModel> m_neighbor_checkpoints;

    Order get_limit_buy_spot_order_by_checkpoint(DataModel& checkpoint);
    TaskVoid handle_price_update(double price);
    TaskVoid handle_order_update(Order& order);
};

#endif //STRATEGY_STATE_MONITORING_H