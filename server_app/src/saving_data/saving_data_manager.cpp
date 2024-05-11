#include <saving_data/saving_data_manager.h>
#include <saving_data/saving_data.h>

#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <data_feed/exchange_connector/binance_connector.h>

#include <exchanges/exchange_gateway.h>

#include <json/json.h>
#include <data_model/data_model.h>

#include <app_constants.h>
#include <utils.h>
#include <app_utils.h>

using namespace std;

void SavingDataManager::start_by_config_from_DB()
{
    // read strategies from DB
    std::vector<std::string> market_name_list =
        MongoDB::instance().get_collection_name_list(SCANNING_MARKET_CONFIG);

    for (auto& market_name : market_name_list)
    {
        std::vector<DataModel> trade_config_list =
            DataModel::get_data_model_list(SCANNING_MARKET_CONFIG, market_name);
        DataModel strategy_data = trade_config_list[0];

        // create strategy
        start_strategy(strategy_data.get_data(), true);
    }
}

ResponseStatusCode SavingDataManager::start_strategy(Json& strategy_inputs, bool is_loaded_from_DB)
{
    long _id = (long)(strategy_inputs["id"]);
    if (m_saving_data_list.find(_id) != m_saving_data_list.end())
        return CREATED_201;

    shared_ptr<SavingData> trading_strategy = nullptr;
    try
    {
        trading_strategy = make_shared<SavingData>(strategy_inputs);
        trading_strategy->start();
    }
    catch(...)
    {
        trading_strategy = nullptr;
        // remove symbols to order book buffer

        return BAD_REQUEST_400;
    }

    if (trading_strategy)
        m_saving_data_list[_id] = trading_strategy;

    return OK_200;
}

ResponseStatusCode SavingDataManager::stop_strategy(const long _id)
{
    if (m_saving_data_list.find(_id) != m_saving_data_list.end())
    {
        shared_ptr<SavingData> trading_strategy =
                m_saving_data_list[_id];
        trading_strategy->stop();

        return OK_200;
    }
    else
        return NOT_FOUND_404;
}

void SavingDataManager::stop_all_strategies()
{
    for(auto trading_strategy : m_saving_data_list)
    {
        trading_strategy.second->stop(true);
    }
}

ResponseStatusCode SavingDataManager::remove_strategy(const long _id)
{
    if (m_saving_data_list.find(_id) != m_saving_data_list.end())
    {
        shared_ptr<SavingData> trading_strategy =
                m_saving_data_list[_id];
        trading_strategy->stop();

        // Drop collection in DB
        MongoDB::instance().drop_collection(SCANNING_MARKET_CONFIG, to_string(_id));

        m_saving_data_list.erase(_id);

        return OK_200;
    }
    else
        return NOT_FOUND_404;
}

ResponseStatusCode SavingDataManager::update_strategy(const long _id,
                                            Json& strategy_input)
{
    if (m_saving_data_list.find(_id) != m_saving_data_list.end())
    {
        // stop current strategy
        // shared_ptr<TradingStrategy> trading_strategy =
        //         m_trading_strategy_list[_id];
        // trading_strategy->stop_trading();

        // update new inputs
        // return m_trading_strategy_list[_id]->update_strategy_inputs(strategy_input);
    }

    return NOT_FOUND_404;
}

Json SavingDataManager::get_all_auto_trading_info()
{
    Json res;
    for (auto it = m_saving_data_list.begin(); it != m_saving_data_list.end(); it++)
    {
        res[to_string(it->first)] =
            m_saving_data_list[it->first]->get_inputs();
    }

    return res;
}

long SavingDataManager::get_last_strategy_id()
{
    long valid_id = 0;

    for (auto it = m_saving_data_list.begin(); it != m_saving_data_list.end(); it++)
    {
        if (valid_id < it->first)
            valid_id = it->first;
    }

    return valid_id;
}
