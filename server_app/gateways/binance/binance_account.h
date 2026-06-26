#pragma once

#include <account/gateway_account.h>

class BinanceAccount : public GatewayAccount<BinanceAccount>
{
    std::string m_api_key;
    std::string m_api_secret;

    virtual ExchangeId get_exchange_id() const override
    {
        return ExchangeId::BINANCE;
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
