#pragma once

#include <string>
#include <json/json.h>

struct StrategyBuySpotConfig
{
    std::string symbol = "BTC-USDT";
    double buy_volumn = 50;
    size_t max_open_orders = 3;
    bool is_running;

    Json to_json()
    {
        return {
            {"symbol", symbol},
            {"buy_volumn", buy_volumn},
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
            res.max_open_orders = (size_t)data["max_open_orders"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
