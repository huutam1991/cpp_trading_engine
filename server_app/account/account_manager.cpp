#include <gateways/gateway_manager.h>

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

        // Add to all_accounts
        get_all_accounts().emplace(account_instance->get_key_name(), account_instance);

        // Add to gateway
        if (account_instance->is_active() == true)
        {
            std::shared_ptr<Gateway> gateway = GatewayManager::instance().get_gateway(exchange_id);
            gateway->add_account(account_instance);
        }
    });
}

std::expected<bool, std::string> AccountManager::set_active_account(const std::string& account_key, bool is_active)
{
    auto& all_accounts = get_all_accounts();

    auto it = all_accounts.find(account_key);
    if (it == all_accounts.end())
    {
        return std::unexpected("Account with key [" + account_key + "] not found");
    }

    it->second->set_active(is_active);
    return true;
}