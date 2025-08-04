#pragma once

#include <variant>

#include <coroutine/task.h>
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

    virtual Task<void> init() = 0;
    virtual Task<void> update(StrategyUpdateData data) = 0;

    // For API requests
    virtual std::string get_name() = 0;
    virtual Json get_config() = 0;
    virtual void update_config(Json& data) = 0;
    virtual Json get_info(Json& params) = 0;
};