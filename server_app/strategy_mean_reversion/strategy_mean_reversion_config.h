#pragma once

#include <string>

#include <json/json.h>
#include <strategy/strategy_config_base.h>

#include "spread_capture_config.h"

struct StrategyMeanReversionConfig : public StrategyConfigBase
{
    double volume = 0.01; // in BTC
    SpreadCaptureConfig spread_capture_config = {2.0, 1.0, 0.8, 2.0};

    Json to_json() const
    {
        Json json = StrategyConfigBase::to_json();
        Json spread_capture_config_json = {
            {"move_distance", spread_capture_config.move_distance},
            {"entry_distance", spread_capture_config.entry_distance},
            {"take_profit", spread_capture_config.take_profit},
            {"stop_loss", spread_capture_config.stop_loss}
        };

        json["volume"] = volume;
        json["spread_capture_config"] = spread_capture_config_json;

        return json;
    }

    static StrategyMeanReversionConfig from_json(Json& data)
    {
        StrategyMeanReversionConfig res;
        StrategyConfigBase* base_config_ptr_of_res = &res;
        *base_config_ptr_of_res = StrategyConfigBase::from_json(data);

        // Only load from [data], if it is valid
        if (data.has_field("symbol"))
        {
            res.volume = (double)data["volume"];

            if (data.has_field("spread_capture_config"))
            {
                Json config_json = data["spread_capture_config"];

                res.spread_capture_config.move_distance = (double)config_json["move_distance"];
                res.spread_capture_config.entry_distance = (double)config_json["entry_distance"];
                res.spread_capture_config.take_profit = (double)config_json["take_profit"];
                res.spread_capture_config.stop_loss = (double)config_json["stop_loss"];
            }
        }

        return res;
    }
};
