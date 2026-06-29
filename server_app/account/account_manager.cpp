
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
        ExchangeId exchange_id = enum_reflect::enum_value<ExchangeId>((std::string)account_json["exchange_id"]);

        auto& account_factory_array = get_account_factory_array();
        std::shared_ptr<AccountBase> account_instance = account_factory_array[exchange_id]();
        account_instance->from_json(account_json);
    });
}