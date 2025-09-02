#pragma once

#include <string>
#include <json/json.h>

struct StrategyTrendFollowConfig
{
    std::string symbol = "BTC-USDC-PERPETUAL";
    double volume = 0.1;
    double ratio = 20.0;
    double price_step = 5.0;
    double take_profit = 20.0;
    bool is_running = false;

    Json to_json() const
    {
        return {
            {"symbol", symbol},
            {"volume", volume},
            {"ratio", ratio},
            {"price_step", price_step},
            {"take_profit", take_profit},
            {"is_running", is_running},
        };
    }

    static StrategyTrendFollowConfig from_json(Json& data)
    {
        StrategyTrendFollowConfig res;

        // Only load from [data], if it is valid
        if (data.has_field("symbol"))
        {
            res.symbol = (std::string)data["symbol"];
            res.volume = (double)data["volume"];
            res.ratio = (double)data["ratio"];
            res.price_step = (double)data["price_step"];
            res.take_profit = (double)data["take_profit"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
