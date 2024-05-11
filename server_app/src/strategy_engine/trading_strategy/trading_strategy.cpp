#include <strategy_engine/trading_strategy/trading_strategy.h>
#include <strategy_engine/hedging_strategy/hedging_strategy.h>

#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <data_feed/data_processor/data_storage/order_book.h>

#include <json/json.h>
#include <utils.h>

using namespace strategy_engine;

TradingStrategy::TradingStrategy(const Json inputs) :
    BaseStrategy(inputs)
{
}

TradingStrategy::~TradingStrategy()
{
}

void TradingStrategy::read_common_inputs()
{
    m_strategy_id = (long)m_strategy_inputs["id"];
    m_exchange_name = (string&&)(m_strategy_inputs["market"]);
    m_scanning_exchange_name = (string&&)(m_strategy_inputs["trade_config"]["exchange"]);
    // get market ids
    m_trading_market_id = get_market_id_from_name(m_exchange_name);
    m_scanning_market_id = get_market_id_from_name(m_scanning_exchange_name);

    m_offset_bid = stold((string&&)m_strategy_inputs["offset_bid"]);
    m_offset_ask = stold((string&&)m_strategy_inputs["offset_ask"]);

    m_bid_quantity = stold((string&&)m_strategy_inputs["bid_qty"]);
    m_ask_quantity = stold((string&&)m_strategy_inputs["ask_qty"]);

    m_is_arm_bid = (string&&)m_strategy_inputs["arm_bid"] == "Yes" ? true : false;
    m_is_arm_ask = (string&&)m_strategy_inputs["arm_ask"] == "Yes" ? true : false;
    m_is_auto_hedge = (string&&)m_strategy_inputs["auto_hedge"] == "Yes" ? true : false;
}

Json TradingStrategy::get_strategy_inputs()
{
    return m_strategy_inputs;
}

void TradingStrategy::init_config_data_model()
{
    std::vector<DataModel> trade_config_list = 
        DataModel::get_data_model_list(SCANNING_MARKET_CONFIG, 
                                    to_string(m_strategy_id));
    if (trade_config_list.size() > 0)
        m_config_data_model = trade_config_list[0];
    else
    {
        // save to DB
        m_config_data_model = DataModel(SCANNING_MARKET_CONFIG, 
                                to_string(m_strategy_id));
        save_strategy_inputs_to_DB();
    }    
}

void TradingStrategy::save_strategy_inputs_to_DB()
{
    m_config_data_model = m_strategy_inputs; 
}

Json TradingStrategy::prepare_sending_data_to_FE(Json& data)
{
    Json ret = data.clone();

    long price_precision = static_cast<long>(m_main_symbol_info["pricePrecision"]);

    ret["id"] = m_strategy_id;
    switch (m_trading_state)
    {
    case STATE_SCANNING:
        ret["trading_status"]  = "scanning";
        break;    
    case STATE_TRADING:
        ret["trading_status"]  = "trading";
        break;
    case STATE_DELETED:
        ret["trading_status"]  = "deleted";
        break;
    default:
        break;
    }

    switch (m_hedging_state)
    {
    case STATE_NONE:
        ret["hedging_status"]  = "none";
        break;    
    case STATE_HEDGING:
        ret["hedging_status"]  = "hedging";
        break;
    case STATE_DELETED:
        ret["hedging_status"]  = "deleted";
        break;
    default:
        break;
    }
    ret["repeat_times"]  = m_repeat_times;

    ret["theo_bid"] = TO_STRING((long double)(data["theo_bid"]), price_precision);
    ret["theo_ask"] = TO_STRING((long double)(data["theo_ask"]), price_precision);
    ret["best_ask"] = TO_STRING((long double)(data["best_ask"]), price_precision);
    ret["best_bid"] = TO_STRING((long double)(data["best_bid"]), price_precision);
    ret["last_price"] = TO_STRING((long double)(data["last_price"]), price_precision);

    return ret;
}

void TradingStrategy::attach_trade_data_info(Json& trade_report)
{
    ADD_LOG("m_trading_data: " << m_trading_data.get_string_value());

    long price_precision = static_cast<long>(m_main_symbol_info["pricePrecision"]);

    trade_report["Theo Bid"] = TO_STRING((long double)(m_trading_data["theo_bid"]), price_precision);
    trade_report["Theo Ask"] = TO_STRING((long double)(m_trading_data["theo_ask"]), price_precision);
    trade_report["Best Bid"] = TO_STRING((long double)(m_trading_data["best_bid"]), price_precision);
    trade_report["Best Ask"] = TO_STRING((long double)(m_trading_data["best_ask"]), price_precision);
    trade_report["Last Price"] = TO_STRING((long double)(m_trading_data["last_price"]), price_precision);

    trade_report["Strategy Name"] = m_strategy_inputs["trade_strategy"];
    trade_report["Strategy ID"] = m_strategy_id;
}

void TradingStrategy::delete_hedging_strategy_if_need()
{
    bool has_hedging_strategy_finished = false;

    for(auto it = m_hedging_strategies.begin(); it != m_hedging_strategies.end();
         ++it)
    {
        if ((*it)->is_hedging_finished())
        {
            (*it)->stop();
            m_hedging_strategies.erase(it);
            has_hedging_strategy_finished = true;
            break;
        }
    }

    // update states
    if (has_hedging_strategy_finished)
    {
        // finish all current hedging orders
        if (m_hedging_strategies.size() == 0)
        {
            if (m_repeat_times > 0)
            {
                m_strategy_inputs["request_type"] = "trade";
                m_trading_state = STATE_TRADING;
            }
            else
            {
                m_strategy_inputs["request_type"] = "scan";
                m_trading_state = STATE_SCANNING;
            }    
            m_hedging_state = STATE_NONE;

            // update to DB
            save_strategy_inputs_to_DB();
        }
    }
}

