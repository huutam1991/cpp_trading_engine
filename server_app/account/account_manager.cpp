
#include "account_manager.h"
#include "account_db.h"
#include "account.h"

std::unordered_map<std::string, std::shared_ptr<AccountBase>>& AccountManager::get_all_accounts()
{
    static std::unordered_map<std::string, std::shared_ptr<AccountBase>> all_accounts;
    return all_accounts;
}

void AccountManager::init()
{
    // Load all accounts from DB
    Json all_accounts = AccountDB::load_all_accounts();

    all_accounts.for_each([&](Json& account_json)
    {
        std::string exchange_id_str = account_json["exchange_id"];
        ExchangeId exchange_id = enum_reflect::enum_value<ExchangeId>(exchange_id_str);

        auto& all_accounts_map = get_all_accounts();
        auto it = all_accounts_map.find(exchange_id_str);
        if (it != all_accounts_map.end())
        {
            std::shared_ptr<AccountBase> account_instance = it->second;
            account_instance->from_json(account_json);
        }
        else
        {
            spdlog::error("AccountManager - init: No registered account class for exchange_id [{}]", exchange_id_str);
        }
    });
}