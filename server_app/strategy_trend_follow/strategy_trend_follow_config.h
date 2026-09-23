#pragma once

#include <string>

#include <json/json.h>
#include <strategy/strategy_config_base.h>

struct StrategyTrendFollowConfig : public StrategyConfigBase
{
    std::string symbol = "BTC-USDC-PERPETUAL";
    double volume = 0.1;
    double ratio = 20.0;
    double price_step = 5.0;
    double take_profit = 20.0;

    Json to_json() const
    {
        Json json = StrategyConfigBase::to_json();
        json["symbol"] = symbol;
        json["volume"] = volume;
        json["ratio"] = ratio;
        json["price_step"] = price_step;
        json["take_profit"] = take_profit;

        return json;
    }

    static StrategyTrendFollowConfig from_json(Json& data)
    {
        StrategyTrendFollowConfig res;
        StrategyConfigBase* base_config_ptr_of_res = &res;
        *base_config_ptr_of_res = StrategyConfigBase::from_json(data);

        // Only load from [data], if it is valid
        if (data.has_field("symbol"))
        {
            res.symbol = (std::string)data["symbol"];
            res.volume = (double)data["volume"];
            res.ratio = (double)data["ratio"];
            res.price_step = (double)data["price_step"];
            res.take_profit = (double)data["take_profit"];
        }

        return res;
    }
};
