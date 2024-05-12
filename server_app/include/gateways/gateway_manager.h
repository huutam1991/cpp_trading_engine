#ifndef GATEWAY_MANAGER_H
#define GATEWAY_MANAGER_H

#include <unordered_map>
#include <memory>

#include <util_macros.h>

#include <gateways/gateway.h>

enum GatewayEnum
{
    BINANCE
};

class GatewayManager
{
    Singleton(GatewayManager);

private:
    std::unordered_map<GatewayEnum, std::shared_ptr<Gateway>> m_gateways;

public:
    void init();
    std::shared_ptr<Gateway> get_gateway(GatewayEnum gateway);
};

#endif //GATEWAY_MANAGER_H