#pragma once

#include <variant>
#include <coroutine/task.h>
#include <data_model/savable_object.h>

#include <gateways/gateway.h>
#include <order/order_manager.h>

#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

// StrategyPriceArbitrageData
struct PriceUpdate
{
    std::string symbol;
    double price;
};
using StrategyPriceArbitrageData = std::variant<PriceUpdate, Order>;

// State data
enum PAState 
{
    PA_RUN,
    PA_STOP
};

struct PAStateData
{
    PAState state = PAState::PA_STOP;

    static inline std::string to_string(PAState data)
    {
        return data == PAState::PA_RUN ? "RUN" : "STOP";
    }

    static inline PAState from_string(const std::string& data)
    {
        return data == "RUN" ? PAState::PA_RUN : PAState::PA_STOP;
    }

    Json to_json()
    {
        return {
            {"state", to_string(state)}
        };
    }

    static PAStateData from_json(Json& data)
    {
        PAStateData res;

        res.state = data.has_field("state") ? 
            from_string((std::string)data["state"]) : PAState::PA_STOP;

        return res;
    }
};

class StrategyPriceArbitrageState
{
protected:
    std::shared_ptr<Gateway>& m_gateway;
    StrategyPriceArbitrageConfig& m_config;

public:
    StrategyPriceArbitrageState(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config);
    ~StrategyPriceArbitrageState();

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyPriceArbitrageData data);

    virtual Json get_open_orders() = 0;

protected:
};
