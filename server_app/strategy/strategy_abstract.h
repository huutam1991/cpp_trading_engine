#pragma once

#include <variant>

#include <coroutine/task_void.h>
#include <coroutine/event_base_manager.h>
#include <order/order.h>

struct PriceUpdate
{
    const Instrument* instrument;
    double price;
};
using StrategyUpdateData = std::variant<PriceUpdate, Order>;

class StrategyAbstract
{
public:
    EventBase* event_base;

public:
    StrategyAbstract(EventBase* v_event_base) : event_base{v_event_base} {}

    virtual TaskVoid init() = 0;
    virtual TaskVoid update(StrategyUpdateData data) = 0;

    // For API requests
    virtual std::string get_name() = 0;
    virtual JsonNew get_config() = 0;
    virtual void update_config(JsonNew& data) = 0;
    virtual JsonNew get_info(JsonNew& params) = 0;
};