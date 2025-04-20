#ifndef STRATEGY_PRICE_ARBITRAGE_CONFIG_H
#define STRATEGY_PRICE_ARBITRAGE_CONFIG_H

#include <string>

struct StrategyPriceArbitrageConfig
{
    std::string base_currency_1;
    std::string base_currency_2;
    std::string quote_currency;
    double buy_volumn;
    double buy_at_lower_price;
    bool is_running;

    std::string get_symbol()
    {
        return base_currency_1 + quote_currency;
    }
};


#endif //STRATEGY_PRICE_ARBITRAGE_CONFIG_H