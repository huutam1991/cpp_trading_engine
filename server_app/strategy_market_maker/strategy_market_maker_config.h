#pragma once

#include <string>

#include <json/json.h>
#include <strategy/strategy_config_base.h>

struct StrategyMarketMakerConfig : public StrategyConfigBase
{
    std::string symbol = "BTC-USDC-PERPETUAL";
    double volumn = 0.1;
    double price_gap = 35.0;
    double min_trade_volume_step = 2.0;
    size_t trade_volume_duration = 900; // In seconds
    double clear_orders_gap = 800.0;

    Json to_json() const
    {
        Json json = StrategyConfigBase::to_json();
        json["symbol"] = symbol;
        json["volumn"] = volumn;
        json["price_gap"] = price_gap;
        json["min_trade_volume_step"] = min_trade_volume_step;
        json["trade_volume_duration"] = trade_volume_duration;
        json["clear_orders_gap"] = clear_orders_gap;

        return json;
    }

    static StrategyMarketMakerConfig from_json(Json& data)
    {
        StrategyMarketMakerConfig res;
        StrategyConfigBase* base_config_ptr_of_res = &res;
        *base_config_ptr_of_res = StrategyConfigBase::from_json(data);

        // Only load from [data], if it is valid
        if (data.has_field("symbol"))
        {
            res.symbol = (std::string)data["symbol"];
            res.volumn = (double)data["volumn"];
            res.price_gap = (double)data["price_gap"];
            res.min_trade_volume_step = (double)data["min_trade_volume_step"];
            res.trade_volume_duration = (size_t)data["trade_volume_duration"];
            res.clear_orders_gap = (double)data["clear_orders_gap"];
        }

        return res;
    }
};
