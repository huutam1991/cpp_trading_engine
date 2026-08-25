#pragma once

#include <string>
#include <json/json.h>

struct StrategyMarketMakerConfig
{
    // Default account is BINANCE_REAL_1
    std::shared_ptr<AccountBase> account = AccountManager::get_account_by_key("BINANCE_REAL_1");
    std::string symbol = "BTC-USDC-PERPETUAL";
    double volumn = 0.1;
    double price_gap = 35.0;
    double min_trade_volume_step = 2.0;
    size_t trade_volume_duration = 900; // In seconds
    double clear_orders_gap = 800.0;
    bool is_running = false;

    Json to_json() const
    {
        return {
            {"account", account != nullptr ? account->get_key_name() : ""},
            {"symbol", symbol},
            {"volumn", volumn},
            {"price_gap", price_gap},
            {"min_trade_volume_step", min_trade_volume_step},
            {"trade_volume_duration", trade_volume_duration},
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
            res.account = AccountManager::get_account_by_key((std::string)data["account"]);
            res.symbol = (std::string)data["symbol"];
            res.volumn = (double)data["volumn"];
            res.price_gap = (double)data["price_gap"];
            res.min_trade_volume_step = (double)data["min_trade_volume_step"];
            res.trade_volume_duration = (size_t)data["trade_volume_duration"];
            res.clear_orders_gap = (double)data["clear_orders_gap"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};
