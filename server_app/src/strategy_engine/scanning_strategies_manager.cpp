#include <strategy_engine/scanning_strategies_manager.h>
#include <strategy_engine/trading_strategy/blvt_scanning_trading_strategy.h>
#include <strategy_engine/trading_strategy/market_making_trading_strategy.h>

#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <data_feed/exchange_connector/binance_connector.h>

#include <exchanges/exchange_gateway.h>

#include <json/json.h>
#include <data_model/data_model.h>

#include <app_constants.h>
#include <utils.h>
#include <app_utils.h>

using namespace std;
using namespace strategy_engine;

void ScanningStrategiesManager::start_by_config_from_DB()
{
    m_exchange_name = BINANCE_SPOT_ABBREVIATION_NAME;

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

ResponseStatusCode ScanningStrategiesManager::start_strategy(Json& strategy_inputs, bool is_loaded_from_DB)
{
    long _id = (long)(strategy_inputs["id"]);
    if (m_trading_strategy_list.find(_id) != m_trading_strategy_list.end())
        return CREATED_201;

    shared_ptr<TradingStrategy> trading_strategy = nullptr;
    try
    {
        if (strategy_inputs["trade_strategy"] == BLVT_TRADING_STRATEGY_UP_NAME ||
            strategy_inputs["trade_strategy"] == BLVT_TRADING_STRATEGY_DOWN_NAME)
        {
            trading_strategy = make_shared<BLVTScanningTradingStrategy>(strategy_inputs);
            trading_strategy->start();
        }
        else if (strategy_inputs["trade_strategy"] == MARKET_MAKING_TRADING_STRATEGY_UP_NAME ||
            strategy_inputs["trade_strategy"] == MARKET_MAKING_TRADING_STRATEGY_DOWN_NAME)
        {
            trading_strategy = make_shared<MarketMakingTradingStrategy>(strategy_inputs);
            trading_strategy->start();
        }
    }
    catch(...)
    {
        trading_strategy = nullptr;
        // remove symbols to order book buffer

        return BAD_REQUEST_400;
    }
    
    if (trading_strategy)
        m_trading_strategy_list[_id] = trading_strategy;

    return OK_200;
}

ResponseStatusCode ScanningStrategiesManager::stop_strategy(const long _id)
{
    if (m_trading_strategy_list.find(_id) != m_trading_strategy_list.end())
    {
        shared_ptr<TradingStrategy> trading_strategy =
                m_trading_strategy_list[_id];
        trading_strategy->stop_trading();

        return OK_200;
    }
    else 
        return NOT_FOUND_404;
}

void ScanningStrategiesManager::stop_all_strategies()
{
    for(auto trading_strategy : m_trading_strategy_list)
    {
        trading_strategy.second->stop_trading(true);
    }
}

ResponseStatusCode ScanningStrategiesManager::remove_strategy(const long _id)
{
    if (m_trading_strategy_list.find(_id) != m_trading_strategy_list.end())
    {
        shared_ptr<TradingStrategy> trading_strategy =
                m_trading_strategy_list[_id];
        Json strategy_inputs = trading_strategy->get_strategy_inputs();
        trading_strategy->stop();

        // Drop collection in DB
        MongoDB::instance().drop_collection(SCANNING_MARKET_CONFIG, to_string(_id));

        m_trading_strategy_list.erase(_id);

        return OK_200;
    }
    else 
        return NOT_FOUND_404;
}

ResponseStatusCode ScanningStrategiesManager::update_strategy(const long _id, 
                                            Json& strategy_input)
{
    if (m_trading_strategy_list.find(_id) != m_trading_strategy_list.end())
    {
        // stop current strategy
        // shared_ptr<TradingStrategy> trading_strategy =
        //         m_trading_strategy_list[_id];
        // trading_strategy->stop_trading();
        
        // update new inputs
        return m_trading_strategy_list[_id]->update_strategy_inputs(strategy_input);
    }
    else 
    {
        return NOT_FOUND_404;
    }
}

Json ScanningStrategiesManager::get_all_auto_trading_info()
{
    Json res;
    for (auto it = m_trading_strategy_list.begin(); it != m_trading_strategy_list.end(); it++)
    {
        res[to_string(it->first)] =
            m_trading_strategy_list[it->first]->get_strategy_inputs();
    }
    
    return res;
}

string ScanningStrategiesManager::get_client_order_id(const long strategy_id)
{
    static int client_id = 0;
    unique_lock lock(m_client_order_id_mutex);
    
    string s = to_string(strategy_id) + "_" + to_string(++client_id);
    return s;
}

long ScanningStrategiesManager::get_last_strategy_id()
{
    long valid_id = 0;

    for (auto it = m_trading_strategy_list.begin(); it != m_trading_strategy_list.end(); it++)
    {
        if (valid_id < it->first)
            valid_id = it->first;
    }

    return valid_id;
}

void ScanningStrategiesManager::attach_trade_data_info(const long strategy_id, Json& trade_report)
{
    unordered_map<long, 
            shared_ptr<strategy_engine::TradingStrategy>>::iterator it;
    it = m_trading_strategy_list.find(strategy_id);
    if (it != m_trading_strategy_list.end())
    {
        it->second->attach_trade_data_info(trade_report);
    }
}
