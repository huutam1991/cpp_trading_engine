#ifndef STRATEGY_MR_STATE_H
#define STRATEGY_MR_STATE_H

#include <variant>
#include <coroutine/task.h>

#include <gateways/gateway.h>
#include <order/order_manager.h>

#include <strategy_mean_reversion/strategy_mean_reversion_config.h>

struct MRPriceUpdate
{
    std::string symbol;
    double price;
};

using StrategyMeanReversionData = std::variant<MRPriceUpdate, Order>;

class StrategyMeanReversionState
{
protected:
    std::shared_ptr<Gateway>& m_gateway;
    StrategyMeanReversionConfig& m_config;

public:
    StrategyMeanReversionState(std::shared_ptr<Gateway>& gateway, StrategyMeanReversionConfig& config);
    ~StrategyMeanReversionState();

    static DataModel& get_state_status();
    static void set_state_status(const std::string& status);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyMeanReversionData data);

    virtual Json get_open_orders() = 0;

protected:
};

#endif //STRATEGY_MR_STATE_H