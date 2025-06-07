#pragma once

#include <variant>
#include <coroutine/task.h>

#include <gateways/gateway.h>
#include <strategy_buy_spot/check_points.h>
#include <order/order_manager.h>

enum StateStatus
{
    START,
    PLACING,
    MONITORING,
    CLOSING,
    STOP_FIRST,
};

using StrategyData = std::variant<double, Order>;

class StrategyStateFirst
{
protected:
    std::shared_ptr<Gateway>& m_gateway;
    std::shared_ptr<CheckPointList>& m_checkpoints;

public:
    StrategyStateFirst(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);
    ~StrategyStateFirst();

    static DataModel& get_state_status();
    static void set_state_status(const std::string& status);
    static void set_placing_price(double price);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);

protected:
    TaskVoid send_close_spot_order(DataModel& checkpoint);
    void send_close_perpetual_order(DataModel& checkpoint);

    Order get_close_buy_spot_order_by_checkpoint(DataModel& checkpoint);
    Order get_close_sell_perpetual_order_by_checkpoint(DataModel& checkpoint);

    // For placing at a specific price
    static double* placing_price_ptr();
    static double get_placing_price();
};
