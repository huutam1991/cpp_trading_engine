#pragma once

#include <coroutine/task_void.h>
#include <order/order.h>

struct PriceUpdate
{
    std::string symbol;
    double price;
};
using StrategyUpdateData = std::variant<PriceUpdate, Order>;

class StategyAbstract
{
public:
    virtual TaskVoid update(StrategyUpdateData data) = 0;
};