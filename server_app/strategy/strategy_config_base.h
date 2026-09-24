#pragma once

#include <json/json.h>
#include <enum_reflect/enum_reflect.h>

#include <account/account.h>
#include <instrument/instrument.h>
#include <order/simulator_order.h>

struct StrategyConfigBase
{
    std::vector<std::shared_ptr<AccountBase>> accounts;
    std::vector<const Instrument*> instruments;
    bool is_real_trading = SimulatorOrder::get_active() ? false : true;
    bool is_running = false;

    void resize(size_t num_accounts, size_t num_instruments)
    {
        accounts.resize(num_accounts);
        instruments.resize(num_instruments);

        for (size_t i = 0; i < num_accounts; i++)
        {
            // Default is BINANCE_REAL_1
            accounts[i] = AccountManager::get_account_by_key("BINANCE_REAL_1");
        }

        for (size_t i = 0; i < num_instruments; i++)
        {
            // Default is BTC-USDC-PERPETUAL
            instruments[i] = Instrument::get_instrument_by_symbol(ExchangeId::BINANCE, "BTC-USDC-PERPETUAL");
        }
    }

    Json to_json() const
    {
        Json accounts_json;
        for (const auto& account : accounts)
        {
            accounts_json.push_back(account->get_key_name());
        }

        Json instruments_json;
        for (const auto& instrument : instruments)
        {
            instruments_json.push_back(instrument->to_json());
        }

        return {
            {"accounts", accounts_json},
            {"instruments", instruments_json},
            {"is_real_trading", is_real_trading},
            {"is_running", is_running},
        };
    }

    template <size_t NUM_ACCOUNTS = 1, size_t NUM_INSTRUMENTS = 1>
    static StrategyConfigBase from_json(Json& data)
    {
        StrategyConfigBase res;
        res.resize(NUM_ACCOUNTS, NUM_INSTRUMENTS);

        // Only load from [data], if it is valid
        if (data.has_field("accounts") && data.has_field("instruments"))
        {
            res.accounts.clear();
            res.instruments.clear();

            for (const auto& account : data["accounts"].array())
            {
                res.accounts.push_back(AccountManager::get_account_by_key((std::string)account));
            }

            for (auto& instrument : data["instruments"].array())
            {
                std::string symbol = instrument["symbol"];
                std::string exchange_id = instrument["exchange_id"];

                res.instruments.push_back(Instrument::get_instrument_by_symbol(
                    enum_reflect::enum_value<ExchangeId>(exchange_id),
                    symbol)
                );
            }

            res.is_real_trading = (bool)data["is_real_trading"];
            res.is_running = (bool)data["is_running"];
        }

        return res;
    }
};