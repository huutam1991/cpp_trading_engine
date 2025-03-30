#ifndef STRATEGY_H
#define STRATEGY_H

#include <mutex>
#include <queue>

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>
#include <coroutine/task.h>
#include <coroutine/future.h>

#include <strategy/check_points.h>
#include <strategy/strategy_state/strategy_state.h>

class Strategy
{
    Singleton(Strategy)

private:
    // Mutex
    std::mutex m_strategy_mutex;

    // Info
    std::string m_symbol;
    double m_buy_volumn;
    double m_move_price;
    double m_sell_buy_ratio;

    // Status
    double m_current_price = -1.0;
    bool m_is_running = false;
    bool m_is_close_all_positions = false;
    bool m_is_init = false;

    // Checkpoints + Gateway
    std::shared_ptr<CheckPointList> m_checkpoints;
    std::shared_ptr<Gateway> m_gateway;

    // StrategyState
    static std::unordered_map<std::string, StrategyState*>* get_strategy_states();

    // Data update
    TaskVoid m_update_task;
    bool m_is_run_update = false;
    std::queue<StrategyData> m_state_data_queue;
    Future<bool>::FutureValue m_has_data_update;
    Future<bool> wait_new_data_update();

    void start();
    void stop();
    void close_all_positions();

public:
    void init();
    void on_config_change();
    TaskVoid update();

    double get_current_price();
    Json   get_current_info();
    DataModel get_checkpoint_by_price(double price);
};

class SimpleGuard
{
    bool* m_value;

public:
    SimpleGuard(bool& value) { m_value = &value; *m_value = true; }
    ~SimpleGuard() { *m_value = false; }
};

#endif //STRATEGY_H