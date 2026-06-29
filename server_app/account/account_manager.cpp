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

std::expected<bool, std::string> AccountManager::add_account(Json& account_json)
{
    ExchangeId exchange_id = enum_reflect::enum_value<ExchangeId>((std::string)account_json["exchange_id"]);

    auto& account_factory_array = get_account_factory_array();
    std::shared_ptr<AccountBase> account_instance = account_factory_array[exchange_id]();
    account_instance->from_json(account_json);

    // Save to DB
    AccountDB::save_account_to_db(account_json);

    // Add to all_accounts
    get_all_accounts().emplace(account_instance->get_key_name(), account_instance);

    // Add to gateway
    if (account_instance->is_active() == true)
    {
        std::shared_ptr<Gateway> gateway = GatewayManager::instance().get_gateway(exchange_id);
        gateway->add_account(account_instance);
    }

    return true;
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
    AccountDB::update_account_in_db(account_key, it->second->to_json());

    // Update gateway
    std::shared_ptr<Gateway> gateway = GatewayManager::instance().get_gateway(it->second->get_exchange_id());
    if (gateway != nullptr)
    {
        if (is_active == true)
        {
            gateway->add_account(it->second);
        }
        else
        {
            gateway->remove_account(it->second);
        }
    }

    return true;
}