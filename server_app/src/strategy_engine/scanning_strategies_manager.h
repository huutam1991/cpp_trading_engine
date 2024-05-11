#ifndef SCANNING_STRATEGIES_MANAGER_H
#define SCANNING_STRATEGIES_MANAGER_H

#include <memory>
#include <unordered_map>
#include <set>
#include <mutex>

#include <util_macros.h>
#include <exchanges/exchange.h>

#include <strategy_engine/trading_strategy/trading_strategy.h>

class Json;

using namespace std;

class ScanningStrategiesManager
{
    Singleton(ScanningStrategiesManager)

public:
    void start_by_config_from_DB();

    ResponseStatusCode start_strategy(Json& strategy_inputs, bool is_loaded_from_DB = false);
    ResponseStatusCode stop_strategy(const long _id);
    void stop_all_strategies();
    ResponseStatusCode remove_strategy(const long _id);
    ResponseStatusCode update_strategy(const long _id, Json& strategy_input);
    Json get_all_auto_trading_info();

    string get_client_order_id(const long strategy_id);
    long get_last_strategy_id();

    void attach_trade_data_info(const long strategy_id, Json& trade_report);

private:
    unordered_map<long, 
            shared_ptr<strategy_engine::TradingStrategy>> m_trading_strategy_list;

    string m_exchange_name;
    mutex m_client_order_id_mutex;
    
    DataCallback m_update_order_book_callback;
};

#endif //SCANNING_STRATEGIES_MANAGER_H
