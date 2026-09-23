#pragma once

#include <json/json.h>
#include <account/account.h>
#include <order/simulator_order.h>

struct StrategyConfigBase
{
    std::shared_ptr<AccountBase> account = AccountManager::get_account_by_key("BINANCE_REAL_1");
    bool is_real_trading = SimulatorOrder::get_active() ? false : true;
    bool is_running = false;

    Json to_json() const
    {
        return {
            {"account", account != nullptr ? account->get_key_name() : ""},
            {"is_real_trading", is_real_trading},
            {"is_running", is_running},
        };
    }

    static StrategyConfigBase from_json(Json& data)
    {
        StrategyConfigBase res;

        // Only load from [data], if it is valid
        if (data.has_field("account"))
        {
            res.account = AccountManager::get_account_by_key((std::string)data["account"]);
            res.is_real_trading = (bool)data["is_real_trading"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};