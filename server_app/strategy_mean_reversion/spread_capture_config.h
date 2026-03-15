#pragma once

#include <stdint.h>

struct SpreadCaptureConfig
{
    double entry_distance = 10.0; // in USD
    double take_profit = 3.0; // in USD
    double stop_loss = 100.0; // in USD

    uint64_t success = 0;
    uint64_t fail = 0;

    double win_rate() const
    {
        return double(success) * 100.0 / double(success + fail);
    }
};