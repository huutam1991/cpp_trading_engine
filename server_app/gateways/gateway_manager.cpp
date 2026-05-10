#include <json/json.h>
#include <mongo_db/mongo_db.h>
#include <app_constants.h>
#include <instrument/instrument.h>
#include <gateways/gateway_manager.h>
#include <gateways/binance/binance_gateway.h>
#include <gateways/coinbase/coinbase_gateway.h>

void GatewayManager::init()
{
    Json activate_accounts = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "activate_accounts")
        .find_many();

    activate_accounts.for_each([this](Json& activate_account)
    {
        std::string exchange = activate_account["exchange"];
        std::string key = activate_account["key"];

        // Skip in-active key
        if (activate_account.has_field("is_active") == false || (bool)activate_account["is_active"] == false)
        {
            return;
        }

        spdlog::debug("Load activated account: {}", activate_account);

        ExchangeId gateway_enum = gateway_name_to_enum(exchange);

        if (gateway_enum == ExchangeId::BINANCE)
        {
            m_gateways.insert(std::make_pair(ExchangeId::BINANCE, std::make_shared<BinanceGateway>(key)));
        }
        else if (gateway_enum == ExchangeId::COINBASE)
        {
            m_gateways.insert(std::make_pair(ExchangeId::COINBASE, std::make_shared<CoinbaseGateway>(key)));
        }
        else
        {
            // TBD
        }

        // Check to init gateway
        if (m_gateways.find(gateway_enum) != m_gateways.end())
        {
            m_gateways[gateway_enum]->init();
        }
    });

    // Subscribe instruments for gateways
    std::vector<const Instrument*> subscribed_instruments = Instrument::get_subscribed_instruments();
    for (const Instrument* instrument : subscribed_instruments)
    {
        if (m_gateways.find(instrument->exchange_id) != m_gateways.end())
        {
            m_gateways[instrument->exchange_id]->subscribe_instruments({instrument});
        }
    }
}

ExchangeId GatewayManager::gateway_name_to_enum(const std::string& gateway)
{
    if (gateway == "binance")
    {
        return ExchangeId::BINANCE;
    }
    else if (gateway == "coinbase")
    {
        return ExchangeId::COINBASE;
    }
    // Default is BINANCE
    else
    {
       return ExchangeId::BINANCE;
    }
}

std::shared_ptr<Gateway> GatewayManager::get_gateway(ExchangeId gateway_enum)
{
    if (m_gateways.find(gateway_enum) != m_gateways.end())
    {
        return m_gateways[gateway_enum];
    }

    return std::shared_ptr<Gateway>(nullptr);
}

std::shared_ptr<Gateway> GatewayManager::get_gateway(const std::string& gateway)
{
    ExchangeId gateway_enum = gateway_name_to_enum(gateway);
    return get_gateway(gateway_enum);
}