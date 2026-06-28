#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include <instrument/instrument.h>

class AccountBase;

struct AccountManager
{
    static std::unordered_map<ExchangeId, std::vector<std::string>>& get_field_names_map()
    {
        static std::unordered_map<ExchangeId, std::vector<std::string>> field_names_map;
        return field_names_map;
    }

    template <class T>
    static void register_account()
    {
        T dummy_object;

        Json dummy_json = dummy_object.to_json();
        std::vector<std::string> field_names;

        dummy_json.for_each_with_key([&](const std::string& key, Json& value)
        {
            field_names.push_back(key);
        });

        ExchangeId exchange_id = dummy_object.get_exchange_id();
        auto& field_names_map = get_field_names_map();
        field_names_map.insert(std::make_pair(exchange_id, field_names));
    }

    static Json get_all_accounts_field_names()
    {
        Json result;

        for (const auto& [exchange_id, field_names] : get_field_names_map())
        {
            Json exchange_json;
            Json field_names_json;

            for (const auto& field_name : field_names)
            {
                field_names_json.push_back(field_name);
            }

            exchange_json["field_names"] = field_names_json;
            exchange_json["exchange_id"] = enum_reflect::enum_name(exchange_id);

            result.push_back(exchange_json);
        }

        return result;
    }

    static std::unordered_map<std::string, std::shared_ptr<AccountBase>>& get_all_accounts();
    static void init();
};
