#pragma once

#include <string>
#include <json/json.h>

struct StrategyMarketMakerConfig
{
    std::string symbol = "BTCUSDT";
    double buy_volumn = 100.0;
    double spread = 10.0;
    bool is_running = false;

    Json to_json()
    {
        return {
            {"symbol", symbol},
            {"buy_volumn", buy_volumn},
            {"spread", spread},
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
            res.spread = (double)data["spread"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
