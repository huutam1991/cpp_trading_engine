#include <strategy_engine/hedging_strategy/bf_hedging_strategy.h>
#include <strategy_engine/scanning_strategies_manager.h>

#include <exchanges/exchange_gateway.h>
#include <asset_manager/binance_asset_manager.h>

#include <json/json.h>

#include <utils.h>
#include <timer.h>
#include <app_utils.h>
#include <binance_utils.h>

using namespace strategy_engine;

BFHedgingStrategy::BFHedgingStrategy(const Json inputs) :
    HedgingStrategy(inputs)
{
    // create exchange connector if need
    m_symbol = (string&&)m_strategy_inputs["symbol"];
    m_exchange_name = (string&&)m_strategy_inputs["exchange"];
    m_market_id = get_market_id_from_name(m_exchange_name);
    m_symbol_info = ExchangeGateWay::instance().get_exchange_info(m_market_id, m_symbol);

    // calculate prices 
    m_side = (string&&)m_strategy_inputs["query_json"]["side"];

    m_is_hedging_finished = false;
    // just open a future order to hedge

    // subscribe symbol data
    m_symbol_list.clear();
    if (m_market_id == BINANCE_SPOT || m_market_id == BINANCE_FUTURES)
        m_symbol_list.push_back(m_exchange_name + "#" + m_symbol);
    else
        m_symbol_list.push_back(m_exchange_name + "#" + (string&&)m_symbol_info["baseAsset"]);
        
}

BFHedgingStrategy::~BFHedgingStrategy()
{
    // close exchange connector
}

void BFHedgingStrategy::on_init() 
{
    open_hedging_order();
}

void BFHedgingStrategy::on_deinit() 
{
}

void BFHedgingStrategy::on_tick() 
{
    open_hedging_order();
}

void BFHedgingStrategy::open_hedging_order()
{
    if (m_is_hedging_finished) return;

    // open market trade
    Json query_json = m_strategy_inputs["query_json"].clone();
    if (m_market_id == BINANCE_BLVT)
    {
        query_json["symbol"] = (string&&)m_symbol_info["baseAsset"];
        if (m_side == "BUY")
        {
            query_json["amount"] = query_json["quantity"];
            query_json.remove_field("quantity");
        }
    }
    else
    {
        string client_order_id = ScanningStrategiesManager::instance().get_client_order_id(m_strategy_id);
        query_json["newClientOrderId"] = client_order_id;
    }

    if (!BinanceAssetManager::instance().check_asset_available(m_market_id, query_json))
    {
        // send to client
        Json err_data;
        err_data["message"] = "Spot asset is not available for SELL: " + (string&&)query_json["symbol"];
        err_data["code"] = NOTIFICATION_STATE_TRADE_ERROR;
        send_data_to_client_through_channel(CHANNEL_SCANNING_MARKET_NOTIFICATION, 
                                            err_data);

        return;
    }

    Json response =         
        ExchangeGateWay::instance().create_order(m_market_id, query_json);

    ADD_LOG("open market hedging order: " << response.get_string_value());

    // update status
    if (response["error"] == false)
    {
        // add trade history
        if (m_market_id == BINANCE_BLVT)
        {
            BinanceAssetManager::instance().query_blvt_record(
                                    (string&&)response["data"]["side"],
                                    (long)response["data"]["orderId"],
                                    m_strategy_id);
        }

        m_current_placed_order_id = response["data"]["orderId"];
        m_is_hedging_finished = true;      
    }
    else 
    {
        // placed order failed -> replace order
        m_is_hedging_finished = false;      

        // send to client
        Json err_data;
        err_data["message"] = response["msg"];
        err_data["code"] = NOTIFICATION_STATE_TRADE_ERROR;
        send_data_to_client_through_channel(CHANNEL_SCANNING_MARKET_NOTIFICATION, 
                                            err_data);
    }
}
