#ifndef STRATEGY_MEAN_REVERSION_H
#define STRATEGY_MEAN_REVERSION_H

#include <mutex>
#include <queue>

#include <util_macros.h>
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
    // Mutex
    std::mutex m_strategy_mutex;

    // Info
    StrategyPriceArbitrageConfig m_config;
    bool m_is_init = false;

    // Gateway
    std::shared_ptr<Gateway> m_gateway;

    // StrategyState
    static std::unordered_map<std::string, StrategyPriceArbitrageState*>* get_strategy_states();

    // Data update
    TaskVoid m_update_task;
    bool m_is_run_update = false;
    std::queue<StrategyPriceArbitrageData> m_state_data_queue;
    Future<bool>::FutureValue m_has_data_update;
    Future<bool> wait_new_data_update();

    void run();
    void stop();

public:
    void init();
    void on_config_change();
    TaskVoid update();

    Json get_orders_chain();
    Json get_open_orders();
};

class MeanReversionSimpleGuard
{
    bool* m_value;

public:
    MeanReversionSimpleGuard(bool& value) { m_value = &value; *m_value = true; }
    ~MeanReversionSimpleGuard() { *m_value = false; }
};

#endif //STRATEGY_MEAN_REVERSION_H