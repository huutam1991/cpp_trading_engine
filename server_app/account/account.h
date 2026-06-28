#pragma once

#include <string>
#include <expected>

#include <json/json.h>
#include <instrument/instrument.h>
#include <gateways/order_entry.h>

#include "account_manager.h"

class AccountBase
{
protected:
    std::string m_key_name;
    ExchangeId m_exchange_id;

    std::shared_ptr<OrderEntry> m_order_entry = nullptr;

public:
    AccountBase(ExchangeId exchange_id) : m_exchange_id(exchange_id) {}

    ExchangeId get_exchange_id() const { return m_exchange_id; }
    virtual Task<std::expected<bool, std::string>> validate_account() = 0;
    virtual Json to_json() const = 0;
    virtual void from_json(Json& data) = 0;
};

template <class T>
class Account : public AccountBase
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

    Account(ExchangeId exchange_id) : AccountBase(exchange_id)
    {
        (void)AccountKey::register_instance; // Ensure the static instance is created
    }
};
