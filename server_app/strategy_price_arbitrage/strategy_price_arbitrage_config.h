#pragma once

#include <string>

#include <json/json.h>
#include <strategy/strategy_config_base.h>

struct StrategyPriceArbitrageConfig : public StrategyConfigBase
{
    std::string symbol_1 = "BTC-USDT";
    std::string symbol_2 = "ETH-BTC";
    std::string symbol_3 = "ETH-USDT";
    double buy_volumn = 50.0;
    double buy_at_lower_price = 10.0;
    double price_delta = 5.0;
    double too_low_price_delta = 5.0;
    double too_high_price_delta = 15.0;

    Json to_json() const
    {
        Json json = StrategyConfigBase::to_json();
        json["symbol_1"] = symbol_1;
        json["symbol_2"] = symbol_2;
        json["symbol_3"] = symbol_3;
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
        *base_config_ptr_of_res = StrategyConfigBase::from_json(data);

        // Only load from [data], if it is valid
        if (data.has_field("symbol_1") && data.has_field("symbol_2") && data.has_field("symbol_3"))
        {
            res.symbol_1 = (std::string)data["symbol_1"];
            res.symbol_2 = (std::string)data["symbol_2"];
            res.symbol_3 = (std::string)data["symbol_3"];
            res.buy_volumn = (double)data["buy_volumn"];
            res.buy_at_lower_price = (double)data["buy_at_lower_price"];
            res.price_delta = (double)data["price_delta"];
            res.too_low_price_delta = (double)data["too_low_price_delta"];
            res.too_high_price_delta = (double)data["too_high_price_delta"];
        }

        return res;
    }
};
