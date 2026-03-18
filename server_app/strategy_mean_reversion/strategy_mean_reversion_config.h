#pragma once

#include <string>
#include "spread_capture_config.h"

struct StrategyMeanReversionConfig
{
    std::string symbol = "BTC-USDC-PERPETUAL"; // BTCUSDC perpetual by default
    bool is_running = false;
    double volume = 0.01; // in BTC
    std::vector<SpreadCaptureConfig> spread_capture_configs = {{2.5, 1.5, 0.8, 2.0}};

    Json to_json() const
    {
        Json spread_capture_configs_json;
        for (const auto& config : spread_capture_configs)
        {
            spread_capture_configs_json.push_back({
                {"move_distance", config.move_distance},
                {"entry_distance", config.entry_distance},
                {"take_profit", config.take_profit},
                {"stop_loss", config.stop_loss}
            });
        }

        return {
            {"symbol", symbol},
            {"is_running", is_running},
            {"volume", volume},
            {"spread_capture_configs", spread_capture_configs_json}
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
            res.volume = (double)data["volume"];

            res.spread_capture_configs = std::vector<SpreadCaptureConfig>();
            if (data.has_field("spread_capture_configs") && data["spread_capture_configs"].is_array())
            {
                data["spread_capture_configs"].for_each([&res](Json& config_json)
                {
                    SpreadCaptureConfig config;
                    config.move_distance = (double)config_json["move_distance"];
                    config.entry_distance = (double)config_json["entry_distance"];
                    config.take_profit = (double)config_json["take_profit"];
                    config.stop_loss = (double)config_json["stop_loss"];

                    res.spread_capture_configs.push_back(config);
                });
            }
        }

        return res;
    }
};
