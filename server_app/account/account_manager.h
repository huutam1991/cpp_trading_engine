#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <expected>

#include <instrument/instrument.h>

class AccountBase;

struct AccountManager
{
    static std::array<std::vector<std::string>, ExchangeId::TOTAL_EXCHANGES>& get_field_names_array()
    {
        static std::array<std::vector<std::string>, ExchangeId::TOTAL_EXCHANGES> field_names_array;
        return field_names_array;
    }

    static std::array<std::function<std::shared_ptr<AccountBase>()>, ExchangeId::TOTAL_EXCHANGES>& get_account_factory_array()
    {
        static std::array<std::function<std::shared_ptr<AccountBase>()>, ExchangeId::TOTAL_EXCHANGES> account_factory_array;
        return account_factory_array;
    }

    template <class T>
    static void register_account()
    {
        // Register field names for the account type T
        T dummy_object;
        Json dummy_json = dummy_object.to_json();
        std::vector<std::string> field_names;

        dummy_json.for_each_with_key([&](const std::string& key, Json& value)
        {
            field_names.push_back(key);
        });

        ExchangeId exchange_id = dummy_object.get_exchange_id();
        auto& field_names_array = get_field_names_array();
        field_names_array[exchange_id] = field_names;

        // Register factory function for creating instances of T
        auto& account_factory_array = get_account_factory_array();
        account_factory_array[exchange_id] = []() -> std::shared_ptr<AccountBase> { return std::make_shared<T>(); };
    }

    static Json get_all_accounts_field_names()
    {
        Json result;

        auto& field_names_array = get_field_names_array();
        for (size_t exchange_id = ExchangeId::NONE_EXCHANGE + 1; exchange_id < ExchangeId::TOTAL_EXCHANGES; exchange_id++)
        {
            ExchangeId exchange_id_enum = static_cast<ExchangeId>(exchange_id);
            const auto& field_names = field_names_array[exchange_id];
            Json exchange_json;
            Json field_names_json;

            for (const auto& field_name : field_names)
            {
                field_names_json.push_back(field_name);
            }

            exchange_json["field_names"] = field_names_json;
            exchange_json["exchange_id"] = enum_reflect::enum_name(exchange_id_enum);

            result.push_back(exchange_json);
        }

        return result;
    }

    static void init();

    static std::unordered_map<std::string, std::shared_ptr<AccountBase>>& get_all_accounts();
    static std::shared_ptr<AccountBase> create_account_from_data(Json& data);
    static std::vector<std::shared_ptr<AccountBase>> get_active_accounts();

    static std::expected<bool, std::string> add_account(Json& account_json);
    static std::expected<bool, std::string> set_active_account(const std::string& account_key, bool is_active);
};
