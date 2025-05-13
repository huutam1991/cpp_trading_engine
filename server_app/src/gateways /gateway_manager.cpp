#include <json/json.h>
#include <mongo_db/mongo_db.h>
#include <app_constants.h>
#include <gateways/gateway_manager.h>
#include <gateways/binance/binance_gateway.h>
#include <gateways/coinbase/coinbase_gateway.h>

void GatewayManager::init()
{
    ADD_LOG("Loading activate accounts:");

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

        GatewayEnum gateway_enum = gateway_name_to_enum(exchange);

        if (gateway_enum == GatewayEnum::BINANCE)
        {
            m_gateways.insert(std::make_pair(GatewayEnum::BINANCE, std::make_shared<BinanceGateway>(key)));
        }
        else if (gateway_enum == GatewayEnum::COINBASE)
        {
            m_gateways.insert(std::make_pair(GatewayEnum::COINBASE, std::make_shared<CoinbaseGateway>(key)));
        }
        else
        {
            // TBD
        }
    });
}

GatewayEnum GatewayManager::gateway_name_to_enum(const std::string& gateway)
{
    if (gateway == "binance")
    {
        return GatewayEnum::BINANCE;
    }
    else if (gateway == "coinbase")
    {
        return GatewayEnum::COINBASE;
    }
    // Default is BINANCE
    else
    {
       return GatewayEnum::BINANCE;
    }
}

std::shared_ptr<Gateway> GatewayManager::get_gateway(GatewayEnum gateway_enum)
{
    if (m_gateways.find(gateway_enum) != m_gateways.end())
    {
        return m_gateways[gateway_enum];
    }

    return std::shared_ptr<Gateway>(nullptr);
}

std::shared_ptr<Gateway> GatewayManager::get_gateway(const std::string& gateway)
{
    GatewayEnum gateway_enum = gateway_name_to_enum(gateway);
    return get_gateway(gateway_enum);
}