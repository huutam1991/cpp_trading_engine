#pragma once

#include <string>
#include <json/json.h>

struct StrategyBuySpotConfig
{
    std::string symbol = "BTC-USDT";
    double buy_volumn = 50;
    double buy_at_lower_price;
    double price_delta;
    double too_low_price_delta;
    double too_high_price_delta;
    bool is_running;

    Json to_json()
    {
        return {
            {"symbol", symbol},
            {"buy_volumn", buy_volumn},
            {"buy_at_lower_price", buy_at_lower_price},
            {"price_delta", price_delta},
            {"too_low_price_delta", too_low_price_delta},
            {"too_high_price_delta", too_high_price_delta},
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
            res.buy_at_lower_price = (double)data["buy_at_lower_price"];
            res.price_delta = (double)data["price_delta"];
            res.too_low_price_delta = (double)data["too_low_price_delta"];
            res.too_high_price_delta = (double)data["too_high_price_delta"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
