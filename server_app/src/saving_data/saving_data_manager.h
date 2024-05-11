#ifndef SAVING_DATA_MANAGER_H
#define SAVING_DATA_MANAGER_H

#include <memory>
#include <unordered_map>
#include <set>
#include <mutex>

#include <util_macros.h>
#include <exchanges/exchange.h>

#include <saving_data/saving_data.h>

class Json;

using namespace std;

class SavingDataManager
{
    Singleton(SavingDataManager)

public:
    void start_by_config_from_DB();

    ResponseStatusCode start_strategy(Json& strategy_inputs, bool is_loaded_from_DB = false);
    ResponseStatusCode stop_strategy(const long _id);
    void stop_all_strategies();
    ResponseStatusCode remove_strategy(const long _id);
    ResponseStatusCode update_strategy(const long _id, Json& strategy_input);
    Json get_all_auto_trading_info();

    long get_last_strategy_id();

private:
    unordered_map<long, 
            shared_ptr<SavingData>> m_saving_data_list;
};

#endif //SAVING_DATA_MANAGER_H
