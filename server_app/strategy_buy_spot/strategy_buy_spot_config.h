#pragma once

#include <string>
#include <json/json.h>

struct StrategyBuySpotConfig
{
    std::string symbol = "BTC-USDT";
    double buy_volumn = 20;
    double move_price = 100;
    double max_price = 10000;
    double min_price = 1000;
    double take_profit = 500;
    size_t max_open_orders = 3;
    bool is_running;

    Json to_json() const
    {
        return {
            {"symbol", symbol},
            {"buy_volumn", buy_volumn},
            {"move_price", move_price},
            {"max_price", max_price},
            {"min_price", min_price},
            {"take_profit", take_profit},
            {"max_open_orders", max_open_orders},
            {"is_running", is_running},
        };
    }

    static StrategyBuySpotConfig from_json(Json& data)
    {
        StrategyBuySpotConfig res;

        // Only load from [data], if it is valid
        if (data.has_field("symbol"))
        {
            res.symbol = (std::string)data["symbol"];
            res.buy_volumn = (double)data["buy_volumn"];
            res.move_price = (double)data["move_price"];
            res.max_price = (double)data["max_price"];
            res.min_price = (double)data["min_price"];
            res.take_profit = (double)data["take_profit"];
            res.max_open_orders = (size_t)data["max_open_orders"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
