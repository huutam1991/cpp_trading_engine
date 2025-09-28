#pragma once

#include <string>
#include <json/json.h>

struct StrategyMarketMakerConfig
{
    std::string symbol = "BTC-USDC-PERPETUAL";
    double volumn = 0.1;
    double price_gap = 35.0;
    double min_trade_volume = 2.0;
    double clear_orders_gap = 800.0;
    bool is_running = false;

    Json to_json() const
    {
        return {
            {"symbol", symbol},
            {"volumn", volumn},
            {"price_gap", price_gap},
            {"min_trade_volume", min_trade_volume},
            {"clear_orders_gap", clear_orders_gap},
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
            res.volumn = (double)data["volumn"];
            res.price_gap = (double)data["price_gap"];
            res.min_trade_volume = (double)data["min_trade_volume"];
            res.clear_orders_gap = (double)data["clear_orders_gap"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
