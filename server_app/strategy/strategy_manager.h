#pragma once

#include <vector>
#include <memory>

#include <utils/util_macros.h>
#include <strategy/strategy_abstract.h>
#include <gateways/gateway_manager.h>
#include <order/order_manager.h>
#include <order_book/order_book_manager.h>

class StrategyManager
{
    Singleton(StrategyManager);

private:
    std::vector<std::unique_ptr<StrategyAbstract>> m_strategy_list;

public:
    void init();
    void add_strategy_list();
    void subscribe_data_update();
    void public_data(StrategyUpdateData& data);

    // For API requests
    Json get_config_by_strategy(const std::string& strategy_name);
    Json update_config_by_strategy(const std::string& strategy_name, Json& data);
    Json get_info(const std::string& strategy_name, Json& params);
};