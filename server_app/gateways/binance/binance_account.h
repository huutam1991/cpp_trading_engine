#pragma once

#include <account/gateway_account.h>

class BinanceAccount : public GatewayAccount<BinanceAccount>
{
    std::string m_api_key;
    std::string m_api_secret;

public:
    BinanceAccount() : GatewayAccount<BinanceAccount>(ExchangeId::BINANCE) {}

    virtual Task<std::expected<bool, std::string>> validate_account() override
    {
        // Implement the logic to validate the Binance account using m_api_key and m_api_secret
        // For example, you can make an API call to Binance to check if the credentials are valid
        co_return std::expected<bool, std::string>(true); // Placeholder for actual validation logic
    }

    virtual Json to_json() const override
    {
        Json data;
        data["exchange_id"] = enum_reflect::enum_name(ExchangeId::BINANCE);
        data["api_key"] = m_api_key;
        data["api_secret"] = m_api_secret;
        data["key"] = m_key_name;

        return data;
    }

    virtual void from_json(Json& data) override
    {
        m_api_key = (std::string)data["api_key"];
        m_api_secret = (std::string)data["api_secret"];
        m_key_name = (std::string)data["key"];
    }
};
