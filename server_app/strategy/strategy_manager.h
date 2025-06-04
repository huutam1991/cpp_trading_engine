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
};