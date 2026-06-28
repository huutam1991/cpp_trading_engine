#pragma once

#include <string>
#include <expected>

#include <json/json.h>
#include <instrument/instrument.h>
#include <gateways/order_entry.h>

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
};

template <class T>
class Account
{
public:
    struct AccountRegister
    {
        AccountRegister()
        {
            AccountManager::register_account<T>();
        }
    };

    struct AccountKey
    {
        static inline AccountRegister register_instance;
    };

    Account(ExchangeId exchange_id) : m_exchange_id(exchange_id)
    {
        (void)AccountKey::register_instance; // Ensure the static instance is created
    }

protected:
    std::string m_key_name;
    ExchangeId m_exchange_id;

    std::shared_ptr<OrderEntry> m_order_entry = nullptr;

public:
    ExchangeId get_exchange_id() const { return m_exchange_id; }
    virtual Task<std::expected<bool, std::string>> validate_account() = 0;
    virtual Json to_json() const = 0;
    virtual void from_json(Json& data) = 0;
};
