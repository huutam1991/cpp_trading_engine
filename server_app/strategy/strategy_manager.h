#pragma once

#include <vector>
#include <memory>

#include <utils/util_macros.h>
#include <strategy/strategy_abstract.h>
#include <gateways/gateway_manager.h>
#include <order/order_manager.h>

class StrategyManager
{
    Singleton(StrategyManager);

private:
    std::vector<std::unique_ptr<StrategyAbstract>> m_strategy_list;

public:
    void init();
    void add_strategy_list();
    void subscribe_data_update();

    // For API requests
    JsonNew get_config_by_strategy(const std::string& strategy_name);
    JsonNew update_config_by_strategy(const std::string& strategy_name, JsonNew& data);
    JsonNew get_info(const std::string& strategy_name, JsonNew& params);
};