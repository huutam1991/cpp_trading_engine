#ifndef STRATEGY_STATE_H
#define STRATEGY_STATE_H

#include <coroutine/task.h>

#include <gateways/gateway.h>
#include <strategy/check_points.h>

enum StateStatus
{
    START,
    PLACING,
    MONITORING,
    CLOSING,
    STOP,
};

class StrategyState
{
protected:
    std::shared_ptr<Gateway>& m_gateway;
    std::shared_ptr<CheckPointList>& m_checkpoints;

public:
    StrategyState(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);
    ~StrategyState();

    static DataModel& get_state_status();
    static void set_state_status(const std::string& status);
    static void set_placing_price(double price);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(double price);

protected:
    void send_close_spot_order(DataModel& checkpoint);
    void send_close_perpetual_order(DataModel& checkpoint);

    Order get_close_buy_spot_order_by_checkpoint(DataModel& checkpoint);
    Order get_close_sell_perpetual_order_by_checkpoint(DataModel& checkpoint);

    // For placing at a specific price
    static double* placing_price_ptr();
    static double get_placing_price();
};

#endif //STRATEGY_STATE_H