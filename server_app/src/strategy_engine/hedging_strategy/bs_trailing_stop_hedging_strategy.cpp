#include <strategy_engine/hedging_strategy/bs_trailing_stop_hedging_strategy.h>
#include <strategy_engine/scanning_strategies_manager.h>

#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <data_feed/data_processor/data_storage/order_book.h>

#include <api_handler/api_handler_binance_spot/api_handler_binance_get_order.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_cancel_replace_order.h>

#include <exchanges/exchange_gateway.h>
#include <asset_manager/binance_asset_manager.h>

#include <json/json.h>

#include <utils.h>
#include <timer.h>
#include <app_utils.h>
#include <binance_utils.h>

using namespace strategy_engine;

BSTrailingStopHedgingStrategy::BSTrailingStopHedgingStrategy(const Json inputs) :
    HedgingStrategy(inputs)
{
    // create exchange connector if need
    m_symbol = (string&&)m_strategy_inputs["symbol"];
    m_exchange_name = (string&&)m_strategy_inputs["exchange"];

    m_symbol_list.push_back(m_exchange_name + "#" + m_symbol);

    m_order_book_buffer = 
        OrderBookManager::instance().get_order_book_by_symbol(m_exchange_name + "#" + m_symbol);
   
    m_price_precision = static_cast<long>(m_strategy_inputs["price_precision"]);
    m_tick_size = static_cast<long double>(m_strategy_inputs["tick_size"]);
    m_trailing_step_tick = static_cast<long>(m_strategy_inputs["trailing_step_tick"]);
    m_trailing_stop_tick = static_cast<long>(m_strategy_inputs["trailing_stop_tick"]);
    m_trailing_limit_tick = static_cast<long>(m_strategy_inputs["trailing_limit_tick"]);

    // calculate prices 
    m_side = (string&&)m_strategy_inputs["query_json"]["side"];
    long double current_price = stold((string&&)m_strategy_inputs["query_json"]["price"]);
    update_new_price(current_price);
}

BSTrailingStopHedgingStrategy::~BSTrailingStopHedgingStrategy()
{
    // close exchange connector
}

void BSTrailingStopHedgingStrategy::set_filled_price(const long double price)
{
    update_new_price(price);
}

void BSTrailingStopHedgingStrategy::on_init() 
{
    m_is_hedging_started = true;
    
    // add tracking order's status - Exchange Gateway
    m_order_manager_callback_id = 
        ExchangeGateWay::instance().subscribe_user_feed(BINANCE_SPOT, [this](Json& order)
    {
        ADD_LOG("hedging_order_callback: - " << order.get_string_value());
        // check hedging order filled
        if ((long)order["orderId"] == m_current_placed_order_id &&
            ((string&&)order["status"] == "FILLED"))
        {
            ADD_LOG("hedging_order_filled_callback: - " << order.get_string_value());
            m_is_hedging_finished = true;
            return;
        }

        // check trader cancel manually
        if ((long)order["orderId"] == m_current_placed_order_id &&
            ((string&&)order["status"] == "CANCELED") &&
            ((string&&)order["clientOrderId"] != m_client_order_id))
        {
            ADD_LOG("hedging_order_canceled_callback: - " << order.get_string_value());
            m_is_hedging_finished = true;
            return;
        }

    });

    update_new_price(m_replace_price);

    // open the first trade
    Json query_json = m_strategy_inputs["query_json"].clone();
    m_client_order_id = ScanningStrategiesManager::instance().get_client_order_id(m_strategy_id);
    query_json["newClientOrderId"] = m_client_order_id;

    if (!BinanceAssetManager::instance().check_asset_available(BINANCE_SPOT, query_json))
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
        ExchangeGateWay::instance().create_order(BINANCE_SPOT, query_json);

    ADD_LOG("open first hedging order: " << response.get_string_value());

    // update status
    if (response["error"] == false)
    {
        m_current_placed_order_id = response["data"]["orderId"];
    }
    else 
    {
        // placed order failed
        if (check_finish_hedging(response))
        {
            m_is_hedging_finished = true;
        }
        m_current_placed_order_id = -1;
    }
}

void BSTrailingStopHedgingStrategy::on_deinit() 
{
    ADD_LOG("BLVTScanningHedgingStrategy::on_deinit: " << m_symbol);
    if (m_order_manager_callback_id != -1)
    {
        ExchangeGateWay::instance().unsubscribe_user_feed(BINANCE_SPOT, 
                                                m_order_manager_callback_id);
    }
}

void BSTrailingStopHedgingStrategy::on_tick() 
{
    if (m_order_book_buffer->get_ring_buffer_size() == 0 || 
        is_hedging_finished())
        return;

    // get symbol buffer
    m_order_book_buffer->begin_read_ring_buffer();
    RingBuffer<Json>* ring_buffer = m_order_book_buffer->read_ring_buffer();
    m_order_book = (*ring_buffer)[0];
    m_order_book_buffer->end_read_ring_buffer();

    // open new hedging order if need
    if (m_side == "SELL" && m_replace_price <= (long double)m_order_book["b"])
    {
        replace_new_order((long double)m_order_book["b"]);
        return;
    }
}

void BSTrailingStopHedgingStrategy::replace_new_order(const long double current_price)
{
    // only one new order at the same time
    std::unique_lock lock(m_strategy_order_mutex);

    // get new price
    update_new_price(current_price);

    // cancel current order
    if (m_current_placed_order_id > 0) 
    {
        m_client_order_id = ScanningStrategiesManager::instance().get_client_order_id(m_strategy_id);
    
        // make new one
        Json query_json = m_strategy_inputs["query_json"].clone();
        query_json["cancelOrderId"] = m_current_placed_order_id;
        query_json["newClientOrderId"] = m_client_order_id;
        
        ADD_LOG("replace hedging query_json: " << query_json.get_string_value());

        if (!BinanceAssetManager::instance().check_asset_available(BINANCE_SPOT, query_json))
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
            ExchangeGateWay::instance().replace_order(BINANCE_SPOT, query_json);

        ADD_LOG("replace hedging order: " << response.get_string_value());

        // place order successful
        if (response["cancelResult"] == "SUCCESS" && response["newOrderResult"] == "SUCCESS")
        {
            m_current_placed_order_id = response["newOrderResponse"]["orderId"];
        }
        // failed to place order -> sell the market
        else 
        {
            query_json.remove_field("newClientOrderId");
            query_json.remove_field("cancelOrderId");
            query_json.remove_field("price");
            query_json.remove_field("stopPrice");
            // place a market order
            query_json["type"] = "MARKET";
        
            Json response =         
                ExchangeGateWay::instance().create_order(BINANCE_SPOT, query_json);

            m_current_placed_order_id = -1;
            m_is_hedging_finished = true;        
        }
    }
}

bool BSTrailingStopHedgingStrategy::is_hedging_finished()
{
    return m_is_hedging_finished;
}

bool BSTrailingStopHedgingStrategy::is_hedging_started()
{
    return m_is_hedging_started;
}

void BSTrailingStopHedgingStrategy::force_stop() 
{
    // only one new order at the same time
    std::unique_lock lock(m_strategy_order_mutex);

    // cancel current order
    if (m_current_placed_order_id > 0) 
    {
        ExchangeGateWay::instance().cancel_order(BINANCE_SPOT, m_symbol, 
                                                m_current_placed_order_id);
    }
    
    // run stop
    stop();
}

void BSTrailingStopHedgingStrategy::update_new_price(const long double current_price)
{
    long double price;
    if (m_side == "BUY")
    {
        m_stop_price = current_price + m_trailing_stop_tick * m_tick_size;
        m_replace_price = current_price - m_trailing_step_tick * m_tick_size;
        price = m_stop_price + m_trailing_limit_tick * m_tick_size;
    }
    else if (m_side == "SELL")
    {
        m_stop_price = current_price - m_trailing_stop_tick * m_tick_size;
        m_replace_price = current_price + m_trailing_step_tick * m_tick_size;
        price = m_stop_price - m_trailing_limit_tick * m_tick_size;
    }

    ADD_LOG("update_new_price: " << m_symbol << " current_price = " << current_price 
            << " stopPrice = " << m_stop_price << " price = " << price);

    // With STOP_LOSS_LIMIT stopPrice != price
    m_strategy_inputs["query_json"]["stopPrice"] = TO_STRING(m_stop_price, m_price_precision);
    m_strategy_inputs["query_json"]["price"] = TO_STRING(price, m_price_precision);
}

bool BSTrailingStopHedgingStrategy::check_finish_hedging(Json& response)
{
    // if (response["error"] == true || response["data"]["status"] == "REJECTED")
    if (response["error"] == true)
    {
        // If order is rejected, finish hedging with a MARKET type order (to make sure order will be filled)
        ADD_LOG("check_order_is_rejected, reponse = " << response);

        Market market = BINANCE_SPOT;
        if (m_exchange_name == BINANCE_FUTURES_ABBREVIATION_NAME)
            market = BINANCE_FUTURES;

        Json query_json = m_strategy_inputs["query_json"].clone();
        query_json.remove_field("cancelOrderId");
        query_json.remove_field("price");
        query_json.remove_field("stopPrice");
        // place a market order
        query_json["type"] = "MARKET";
    
        Json response =         
            ExchangeGateWay::instance().create_order(market, query_json);

        return true;
    }

    return false;
}
