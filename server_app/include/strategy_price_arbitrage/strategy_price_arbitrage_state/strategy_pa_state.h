#ifndef STRATEGY_PA_STATE_H
#define STRATEGY_PA_STATE_H

#include <variant>
#include <coroutine/task.h>

#include <gateways/gateway.h>
#include <order/order_manager.h>

#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

using StrategyData = std::variant<double, Order>;

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
    virtual TaskVoid run(StrategyData data);

protected:
};

#endif //STRATEGY_PA_STATE_H