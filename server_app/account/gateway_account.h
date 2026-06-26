#pragma once

#include <string>

#include <json/json.h>
#include <instrument/instrument.h>

template <class T>
class GatewayAccount
{
public:
    struct GatewayAccountManager
    {
        static std::unordered_map<ExchangeId, std::vector<std::string>>& get_field_names_map()
        {
            static std::unordered_map<ExchangeId, std::vector<std::string>> field_names_map;
            return field_names_map;
        }

        static void register_account()
        {
            Json dummy_object = T::to_json();
            std::vector<std::string> field_names;

            dummy_object.for_each_with_key([&](const std::string& key, Json& value)
            {
                field_names.push_back(key);
            });

            ExchangeId exchange_id = dummy_object.get_exchange_id();
            auto& field_names_map = get_field_names_map();
            field_names_map.insert(std::make_pair(exchange_id, field_names));
        }
    };

    struct GatewayAccountRegister
    {
        GatewayAccountRegister()
        {
            GatewayAccountManager::register_account();
        }
    };

    struct GatewayAccountKey
    {
        static inline GatewayAccountRegister register_instance;
    };

    GatewayAccount(const std::string& key)
    {
        (void)GatewayAccountKey::register_instance; // Ensure the static instance is created
    }

protected:
    std::string m_key_name;

public:
    virtual ExchangeId get_exchange_id() const = 0;
    virtual Json to_json() const = 0;
    virtual void from_json(Json& data) = 0;
};
