#pragma once

#include <string>

struct StrategyPriceArbitrageConfig
{
    std::string symbol_1; // BTCUSDT
    std::string symbol_2; // ETHBTC
    std::string symbol_3; // ETHUSDT
    double buy_volumn;
    double buy_at_lower_price;
    double price_delta;
    double too_low_price_delta;
    double too_high_price_delta;
    bool is_running;
};
