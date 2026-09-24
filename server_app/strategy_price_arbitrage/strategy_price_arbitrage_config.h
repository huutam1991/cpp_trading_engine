#pragma once

#include <string>

#include <json/json.h>
#include <strategy/strategy_config_base.h>

struct StrategyPriceArbitrageConfig : public StrategyConfigBase
{
    double buy_volumn = 50.0;
    double buy_at_lower_price = 10.0;
    double price_delta = 5.0;
    double too_low_price_delta = 5.0;
    double too_high_price_delta = 15.0;

    Json to_json() const
    {
        Json json = StrategyConfigBase::to_json();
        json["buy_volumn"] = buy_volumn;
        json["buy_at_lower_price"] = buy_at_lower_price;
        json["price_delta"] = price_delta;
        json["too_low_price_delta"] = too_low_price_delta;
        json["too_high_price_delta"] = too_high_price_delta;

        return json;
    }

    static StrategyPriceArbitrageConfig from_json(Json& data)
    {
        StrategyPriceArbitrageConfig res;
        StrategyConfigBase* base_config_ptr_of_res = &res;
        *base_config_ptr_of_res = StrategyConfigBase::from_json<1, 3>(data);

        // Only load from [data], if it is valid
        if (data.has_field("buy_volumn") && data.has_field("buy_at_lower_price") && data.has_field("price_delta"))
        {
            res.buy_volumn = (double)data["buy_volumn"];
            res.buy_at_lower_price = (double)data["buy_at_lower_price"];
            res.price_delta = (double)data["price_delta"];
            res.too_low_price_delta = (double)data["too_low_price_delta"];
            res.too_high_price_delta = (double)data["too_high_price_delta"];
        }

        return res;
    }
};
