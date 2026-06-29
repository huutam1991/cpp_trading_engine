#include <json/json.h>
#include <mongo_db/mongo_db.h>
#include <app_constants.h>
#include <instrument/instrument.h>
#include <gateways/gateway_manager.h>
#include <gateways/binance/binance_gateway.h>
#include <gateways/coinbase/coinbase_gateway.h>

void GatewayManager::init()
{
    m_gateways.insert(std::make_pair(ExchangeId::BINANCE, std::make_shared<BinanceGateway>()));
    m_gateways.insert(std::make_pair(ExchangeId::COINBASE, std::make_shared<CoinbaseGateway>()));

    // Check to init gateway
    for (auto& [_, gateway] : m_gateways)
    {
        gateway->init();
    }

    // Subscribe instruments for gateways
    std::vector<const Instrument*> subscribed_instruments = Instrument::get_subscribed_instruments();
    for (const Instrument* instrument : subscribed_instruments)
    {
        if (m_gateways.find(instrument->exchange_id) != m_gateways.end())
        {
            m_gateways[instrument->exchange_id]->subscribe_instrument(instrument);
        }
    }
}

std::vector<std::shared_ptr<Gateway>> GatewayManager::get_all_gateways()
{
    std::vector<std::shared_ptr<Gateway>> gateways;

    for (const auto& [exchange_id, gateway] : m_gateways)
    {
        gateways.push_back(gateway);
    }

    return gateways;
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
    return get_gateway(enum_reflect::enum_value<ExchangeId>(gateway));
}