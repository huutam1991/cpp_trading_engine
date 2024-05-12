#include <json/json.h>
#include <mongo_db/mongo_db.h>
#include <app_constants.h>
#include <gateways/gateway_manager.h>
#include <gateways/binance/binance_gateway.h>

void GatewayManager::init()
{
    ADD_LOG("Loading activate accounts:");

    Json activate_accounts = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "activate_account")
        .find_many();

    activate_accounts.for_each([this](Json& activate_account)
    {
        std::string exchange = activate_account["exchange"];
        std::string key = activate_account["key"];

        if (exchange == "binance")
        {
            m_gateways.insert(std::make_pair(GatewayEnum::BINANCE, std::make_shared<BinanceGateway>(key)));
        }
        else
        {
            // TBD
        }
    });
}

std::shared_ptr<Gateway> GatewayManager::get_gateway(GatewayEnum gateway_enum)
{
    if (m_gateways.find(gateway_enum) != m_gateways.end())
    {
        return m_gateways[gateway_enum];
    }

    return std::shared_ptr<Gateway>(nullptr);
}