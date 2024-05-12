#include <gateways/gateway_manager.h>

std::shared_ptr<Gateway> GatewayManager::get_gateway(GatewayEnum gateway_enum)
{
    if (m_gateways.find(gateway_enum) != m_gateways.end())
    {
        return m_gateways[gateway_enum];
    }

    return std::shared_ptr<Gateway>(nullptr);
}