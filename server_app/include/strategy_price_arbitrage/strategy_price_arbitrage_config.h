#ifndef STRATEGY_PRICE_ARBITRAGE_CONFIG_H
#define STRATEGY_PRICE_ARBITRAGE_CONFIG_H

#include <string>

struct StrategyPriceArbitrageConfig
{
    std::string symbol_1; // BTCUSDT
    std::string symbol_2; // ETHBTC
    std::string symbol_3; // ETHUSDT
    double buy_volumn;
    double buy_at_lower_price;
    bool is_running;
};


#endif //STRATEGY_PRICE_ARBITRAGE_CONFIG_H