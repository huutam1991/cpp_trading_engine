#pragma once

#include <variant>
#include <coroutine/task.h>

#include <gateways/gateway.h>
#include <order/order_manager.h>

#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

struct PriceUpdate
{
    std::string symbol;
    double price;
};

using StrategyPriceArbitrageData = std::variant<PriceUpdate, Order>;

class StrategyPriceArbitrageState
{
protected:
    std::shared_ptr<Gateway>& m_gateway;
    StrategyPriceArbitrageConfig& m_config;

public:
    StrategyPriceArbitrageState(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config);
    ~StrategyPriceArbitrageState();

    static DataModel& get_state_status();
    static void set_state_status(const std::string& status);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyPriceArbitrageData data);

    virtual Json get_open_orders() = 0;

protected:
};
