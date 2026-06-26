#pragma once

#include <account/gateway_account.h>

class CoinbaseAccount : public GatewayAccount<CoinbaseAccount>
{
    std::string m_api_key;
    std::string m_api_secret;
    std::string m_api_passphrase;

public:
    virtual ExchangeId get_exchange_id() const override
    {
        return ExchangeId::COINBASE;
    }

    virtual Json to_json() const override
    {
        Json data;
        data["exchange_id"] = enum_reflect::enum_name(ExchangeId::COINBASE);
        data["api_key"] = m_api_key;
        data["api_secret"] = m_api_secret;
        data["api_passphrase"] = m_api_passphrase;
        data["key"] = m_key_name;

        return data;
    }

    virtual void from_json(Json& data) override
    {
        m_api_key = (std::string)data["api_key"];
        m_api_secret = (std::string)data["api_secret"];
        m_api_passphrase = (std::string)data["api_passphrase"];
        m_key_name = (std::string)data["key"];
    }
};
