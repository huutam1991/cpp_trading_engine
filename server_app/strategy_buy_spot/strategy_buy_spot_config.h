#pragma once

#include <string>

#include <json/json.h>
#include <strategy/strategy_config_base.h>

struct StrategyBuySpotConfig : public StrategyConfigBase
{
    std::string symbol = "BTC-USDT";
    double buy_volumn = 20;
    double move_price = 100;
    double max_price = 10000;
    double min_price = 1000;
    double take_profit = 500;
    size_t max_open_orders = 3;

    Json to_json() const
    {
        Json json = StrategyConfigBase::to_json();

        json["symbol"] = symbol;
        json["buy_volumn"] = buy_volumn;
        json["move_price"] = move_price;
        json["max_price"] = max_price;
        json["min_price"] = min_price;
        json["take_profit"] = take_profit;
        json["max_open_orders"] = max_open_orders;

        return json;
    }

    static StrategyBuySpotConfig from_json(Json& data)
    {
        StrategyBuySpotConfig res;
        StrategyConfigBase* base_config_ptr_of_res = &res;
        *base_config_ptr_of_res = StrategyConfigBase::from_json(data);

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
        }

        return res;
    }
};
