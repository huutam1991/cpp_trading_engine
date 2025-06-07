#pragma once

#include <string>
#include <json/json.h>

struct StrategyPriceArbitrageConfig
{
    std::string symbol_1 = "BTCUSDT"; 
    std::string symbol_2 = "ETHBTC"; 
    std::string symbol_3 = "ETHUSDT"; 
    double buy_volumn = 50.0;
    double buy_at_lower_price = 10.0;
    double price_delta = 5.0;
    double too_low_price_delta = 5.0;
    double too_high_price_delta = 15.0;
    bool is_running = false;

    Json to_json()
    {
        return {
            {"symbol_1", symbol_1},
            {"symbol_2", symbol_2},
            {"symbol_3", symbol_3},
            {"buy_volumn", buy_volumn},
            {"buy_at_lower_price", buy_at_lower_price},
            {"price_delta", price_delta},
            {"too_low_price_delta", too_low_price_delta},
            {"too_high_price_delta", too_high_price_delta},
            {"is_running", is_running},
        };
    }

    static StrategyPriceArbitrageConfig from_json(Json& data)
    {
        StrategyPriceArbitrageConfig res;

        // Only load from [data], if it is valid
        if (data.has_field("symbol_1") && data.has_field("symbol_2") && data.has_field("symbol_3"))
        {
            res.symbol_1 = (std::string)data["symbol_1"];
            res.symbol_2 = (std::string)data["symbol_2"];
            res.symbol_3 = (std::string)data["symbol_3"];
            res.buy_volumn = (double)data["buy_volumn"];
            res.buy_at_lower_price = (double)data["buy_at_lower_price"];
            res.price_delta = (double)data["price_delta"];
            res.too_low_price_delta = (double)data["too_low_price_delta"];
            res.too_high_price_delta = (double)data["too_high_price_delta"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
