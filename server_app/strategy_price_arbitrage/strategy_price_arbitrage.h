#pragma once

#include <mutex>
#include <queue>

#include <utils/util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>
#include <coroutine/task.h>
#include <coroutine/future.h>

#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state.h>

class StrategyPriceArbitrage
{
    Singleton(StrategyPriceArbitrage)

private:
    // Info
    StrategyPriceArbitrageConfig m_config;
    bool m_is_init = false;

    // Gateway
    std::shared_ptr<Gateway> m_gateway;

    // StrategyState
    SavableObject<PAStateData> m_current_state = SavableObject<PAStateData>::load_single_object(STRATEGY_DB_NAME, "price_arbitrage_status");
    PAState m_previous_state;
    static std::unordered_map<PAState, StrategyPriceArbitrageState*>* get_strategy_states();

    void run();
    void stop();

public:
    void init();
    void on_config_change();
    TaskVoid update(StrategyPriceArbitrageData data);

    Json get_orders_chain();
    Json get_open_orders();
};

class PriceArbitrageSimpleGuard
{
    bool* m_value;

public:
    PriceArbitrageSimpleGuard(bool& value) { m_value = &value; *m_value = true; }
    ~PriceArbitrageSimpleGuard() { *m_value = false; }
};
