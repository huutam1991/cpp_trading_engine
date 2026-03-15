#pragma once

#include <stdint.h>
#include <order/order.h>

struct SpreadCaptureConfig
{
    double entry_distance = 10.0; // in USD
    double take_profit = 3.0; // in USD
    double stop_loss = 100.0; // in USD

    uint64_t success = 0;
    uint64_t fail = 0;

    Order buy_order = nullptr;
    Order sell_order = nullptr;

    enum Status
    {
        NONE,
        PLACING_INIT_ORDERS,
        PLACING_HEDGE_BUY_ORDER,
        PLACING_HEDGE_SELL_ORDER,
    };

    Status status = Status::NONE;

    double win_rate() const
    {
        return double(success) * 100.0 / double(success + fail);
    }
};