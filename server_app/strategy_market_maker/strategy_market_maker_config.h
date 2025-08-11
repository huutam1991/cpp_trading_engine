#pragma once

#include <string>
#include <json/json.h>

struct StrategyMarketMakerConfig
{
    std::string symbol = "BTC-USDC";
    double buy_volumn = 0.1;
    double price_gap = 20.0;
    double inventory_skew_ratio = 3.0;
    bool is_running = false;

    Json to_json()
    {
        return {
            {"symbol", symbol},
            {"buy_volumn", buy_volumn},
            {"price_gap", price_gap},
            {"is_running", is_running},
        };
    }

    static StrategyMarketMakerConfig from_json(Json& data)
    {
        StrategyMarketMakerConfig res;

        // Only load from [data], if it is valid
        if (data.has_field("symbol"))
        {
            res.symbol = (std::string)data["symbol"];
            res.buy_volumn = (double)data["buy_volumn"];
            res.price_gap = (double)data["price_gap"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
