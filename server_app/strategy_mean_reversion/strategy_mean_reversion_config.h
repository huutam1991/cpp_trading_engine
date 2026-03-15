#pragma once

#include <string>

struct StrategyMeanReversionConfig
{
    std::string symbol; // BTCUSDT
    bool is_running;

    Json to_json() const
    {
        return {
            {"symbol", symbol},
            {"is_running", is_running},
        };
    }

    static StrategyMeanReversionConfig from_json(Json& data)
    {
        StrategyMeanReversionConfig res;

        // Only load from [data], if it is valid
        if (data.has_field("symbol"))
        {
            res.symbol = (std::string)data["symbol"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
