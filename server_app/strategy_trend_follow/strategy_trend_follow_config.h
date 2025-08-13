#pragma once

#include <string>
#include <json/json.h>

struct StrategyTrendFollowConfig
{
    std::string symbol = "BTC-USDC-PERPETUAL";
    double volumn = 0.1;
    double price_gap = 5.0;
    bool is_running = false;

    Json to_json() const
    {
        return {
            {"symbol", symbol},
            {"volumn", volumn},
            {"price_gap", price_gap},
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
            res.volumn = (double)data["volumn"];
            res.price_gap = (double)data["price_gap"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
