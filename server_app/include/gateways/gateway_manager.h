#pragma once

#include <unordered_map>
#include <memory>

#include <utils/util_macros.h>

#include <gateways/gateway.h>

enum GatewayEnum
{
    BINANCE,
    COINBASE
};

class GatewayManager
{
    Singleton(GatewayManager);

private:
    std::unordered_map<GatewayEnum, std::shared_ptr<Gateway>> m_gateways;

    GatewayEnum gateway_name_to_enum(const std::string& gateway);

public:
    void init();
    std::shared_ptr<Gateway> get_gateway(GatewayEnum gateway);
    std::shared_ptr<Gateway> get_gateway(const std::string& gateway);
};