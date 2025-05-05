#ifndef STRATEGY_MEAN_REVERSION_CONFIG_H
#define STRATEGY_MEAN_REVERSION_CONFIG_H

#include <string>

struct StrategyMeanReversionConfig
{
    std::string symbol; // BTCUSDT
    double buy_volumn;
    double buy_at_lower_price;
    double sell_at_higher_price;
    double too_low_price_delta;
    double too_high_price_delta;
    bool is_running;
};


#endif //STRATEGY_MEAN_REVERSION_CONFIG_H