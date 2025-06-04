#pragma once

#include <coroutine/task_void.h>
#include <coroutine/event_base_manager.h>
#include <order/order.h>

struct PriceUpdate
{
    std::string symbol;
    double price;
};
using StrategyUpdateData = std::variant<PriceUpdate, Order>;

class StrategyAbstract
{
public:
    EventBase* event_base;

public:
    StrategyAbstract(EventBase* v_event_base) : event_base{v_event_base} {}
    virtual TaskVoid update(StrategyUpdateData data) = 0;
};