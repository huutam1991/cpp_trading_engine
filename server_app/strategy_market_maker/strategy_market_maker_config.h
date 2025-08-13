#pragma once

#include <string>
#include <json/json.h>

struct StrategyMarketMakerConfig
{
    std::string symbol = "BTC-USDC-PERPETUAL";
    double volumn = 0.1;
    double price_gap = 20.0;
    double price_step_between_blocks = 100.0;
    int orders_each_side_per_block = 5;
    double inventory_skew_ratio = 3.0;
    double widen = 1.8;
    double tight = 0.6;
    double mom_window_alpha = 0.2;
    double mom_z_reduce     = 3.0;
    double mom_z_pause      = 5.0;
    double mom_widen_mult   = 1.6;
    bool is_running = false;

    Json to_json() const
    {
        return {
            {"symbol", symbol},
            {"volumn", volumn},
            {"price_gap", price_gap},
            {"price_step_between_blocks", price_step_between_blocks},
            {"orders_each_side_per_block", orders_each_side_per_block},
            {"inventory_skew_ratio", inventory_skew_ratio},
            {"widen", widen},
            {"tight", tight},
            {"mom_window_alpha", mom_window_alpha},
            {"mom_z_reduce", mom_z_reduce},
            {"mom_z_pause", mom_z_pause},
            {"mom_widen_mult", mom_widen_mult},
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
            res.price_step_between_blocks = (double)data["price_step_between_blocks"];
            res.orders_each_side_per_block = (int)data["orders_each_side_per_block"];
            res.inventory_skew_ratio = (double)data["inventory_skew_ratio"];
            res.widen = (double)data["widen"];
            res.tight = (double)data["tight"];
            res.mom_window_alpha = (double)data["mom_window_alpha"];
            res.mom_z_reduce = (double)data["mom_z_reduce"];
            res.mom_z_pause = (double)data["mom_z_pause"];
            res.mom_widen_mult = (double)data["mom_widen_mult"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
