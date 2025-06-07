#pragma once

#include <strategy_buy_spot/strategy_buy_spot_state/strategy_state.h>

class StrategyStateMonitoring : public StrategyStateFirst
{
public:
    StrategyStateMonitoring(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);

private:
    std::unordered_map<OrderId, DataModel> m_checkpoint_by_open_order_id;
    double m_take_profit;
    double m_max_price_to_place;

    void remove_open_order_id(OrderId order_id);

    Order get_limit_buy_spot_order_by_checkpoint(DataModel& checkpoint);
    Order get_limit_sell_spot_order_by_checkpoint(DataModel& checkpoint);

    TaskVoid handle_price_update(double price);
    TaskVoid handle_order_update(Order& order);

    TaskVoid check_place_buy_order(double price);
    TaskVoid check_place_sell_order(double price);
};
