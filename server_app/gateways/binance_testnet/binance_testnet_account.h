#pragma once

#include <account/account.h>

class BinanceTestnetAccount : public Account<BinanceTestnetAccount>
{
    std::string m_api_key;
    std::string m_api_secret;

public:
    BinanceTestnetAccount() : Account<BinanceTestnetAccount>(ExchangeId::BINANCE_TESTNET) {}

    std::string get_api_key() const { return m_api_key; }
    std::string get_api_secret() const { return m_api_secret; }

    virtual Task<std::expected<bool, std::string>> validate_account() override
    {
        co_return std::expected<bool, std::string>(true);
    }

    virtual Json to_json() const override
    {
        Json data;
        data["exchange_id"] = enum_reflect::enum_name(ExchangeId::BINANCE_TESTNET);
        data["api_key"] = m_api_key;
        data["api_secret"] = m_api_secret;
        data["key"] = m_key_name;
        data["is_active"] = m_is_active;

        return data;
    }

    virtual void from_json(Json& data) override
    {
        m_api_key = (std::string)data["api_key"];
        m_api_secret = (std::string)data["api_secret"];
        m_key_name = (std::string)data["key"];
        m_is_active = (bool)data["is_active"];
    }
};
