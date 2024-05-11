#include <strategy_engine/trading_strategy/market_making_trading_strategy.h>
#include <strategy_engine/hedging_strategy/bs_trailing_stop_hedging_strategy.h>
#include <strategy_engine/hedging_strategy/bf_hedging_strategy.h>
#include <strategy_engine/scanning_strategies_manager.h>
#include <strategy_engine/base_strategy.h>

#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <data_feed/data_processor/data_storage/order_book.h>

#include <api_handler/api_handler_binance_spot/api_handler_binance_cancel_replace_order.h>

#include <strategy_engine/blvt_scanning/scan_blvt_info.h>

#include <exchanges/exchange_gateway.h>
#include <asset_manager/binance_asset_manager.h>

#include <json/json.h>
#include <binance_utils.h>
#include <app_utils.h>
#include <utils.h>
#include <util_macros.h>
#include <timer.h>

using namespace strategy_engine;

MarketMakingTradingStrategy::MarketMakingTradingStrategy(const Json inputs) :
    TradingStrategy(inputs)
{
    m_trading_state = STATE_SCANNING;
    m_hedging_state = STATE_NONE;

    // add tracking order's status - Exchange Gateway
    m_order_manager_callback_id = 
        ExchangeGateWay::instance().subscribe_user_feed(BINANCE_SPOT, 
                                                        [this](Json& order)
    {
        process_execution_report(order);
    });
}

MarketMakingTradingStrategy::~MarketMakingTradingStrategy()
{
}

// init strategy, init event handler.
void MarketMakingTradingStrategy::on_init()
{
    m_opened_order_id = 0;

    // get initial values
    read_strategy_inputs();

    // Leverage token
    m_main_symbol = string((string&&)m_strategy_inputs["asset"]);
    // BTCUSDT
    m_currency_symbol = string((string&&)m_strategy_inputs["trade_config"]["scan_symbol"]);

    // get symbol info
    m_main_symbol_info = ExchangeGateWay::instance().get_exchange_info(m_trading_market_id, m_main_symbol);
    m_currency_symbol_info = ExchangeGateWay::instance().get_exchange_info(m_scanning_market_id, m_currency_symbol);

    m_symbol_list.clear();
    if (m_trading_market_id == BINANCE_SPOT || m_trading_market_id == BINANCE_FUTURES)
        m_symbol_list.push_back(m_exchange_name + "#" + m_main_symbol);
    else
        m_symbol_list.push_back(m_exchange_name + "#" + (string&&)m_main_symbol_info["baseAsset"]);
        
    if (m_scanning_market_id == BINANCE_SPOT || m_scanning_market_id == BINANCE_FUTURES)
        m_symbol_list.push_back(m_scanning_exchange_name + "#" + m_currency_symbol);
    else
        m_symbol_list.push_back(m_scanning_exchange_name + "#" + (string&&)m_currency_symbol_info["baseAsset"]);

    add_symbols_to_order_book();

    // got data from ring buffer
    m_main_order_book_buffer = 
        OrderBookManager::instance().get_order_book_by_symbol(m_symbol_list[0]);
    m_currency_order_book_buffer = 
        OrderBookManager::instance().get_order_book_by_symbol(m_symbol_list[1]);

    // calculate quantities 
    reset_trade_params();

    // get data model
    init_config_data_model();

    // tracking blvt info
    ScanBLVTInfo::instance().subscribe_symbol(m_main_symbol_info["baseAsset"]);
}

// function is called during deinitialization and is the deinit event handler. 
void MarketMakingTradingStrategy::on_deinit() 
{
    remove_symbols_from_order_book();

    // untracking blvt info
    ScanBLVTInfo::instance().unsubscribe_symbol(m_main_symbol_info["baseAsset"]);

    // Remove tracking order's status
    if (m_order_manager_callback_id != -1)
    {
        ExchangeGateWay::instance().unsubscribe_user_feed(m_trading_market_id, 
                                                            m_order_manager_callback_id);
    }

    m_trading_state = STATE_DELETED;
    m_hedging_state = STATE_DELETED;

    // send stop to client
    Json data = {
        {"status", "deleted"},
        {"id", m_strategy_id}
    };
    send_data_to_client_through_channel(CHANNEL_SCANNING_MARKET, data);
}

// event is generated when a new tick for a symbol is received
void MarketMakingTradingStrategy::on_tick() 
{
    if (m_main_order_book_buffer->get_ring_buffer_size() == 0 ||
        m_currency_order_book_buffer->get_ring_buffer_size() == 0) 
        return;

    m_tick_counter++;
    
    // delete hedging strategy if need
    delete_hedging_strategy_if_need();

    // get data from buffers
    m_main_order_book_buffer->begin_read_ring_buffer();
    RingBuffer<Json>* ring_buffer = m_main_order_book_buffer->read_ring_buffer();
    m_main_order_book = (*ring_buffer)[0];
    m_main_order_book_buffer->end_read_ring_buffer();

    m_currency_order_book_buffer->begin_read_ring_buffer();
    ring_buffer = m_currency_order_book_buffer->read_ring_buffer();
    m_currency_order_book = (*ring_buffer)[0];
    m_currency_order_book_buffer->end_read_ring_buffer();

    // calculate trading data
    m_trading_data = calculate_trading_data();

    // check trigger condition if in trading state
    if (m_trading_state == STATE_TRADING)
    if ((!m_is_arm_ask || !m_is_arm_bid) && (m_is_arm_ask | m_is_arm_bid))
    {
        // check market making condition
        if (can_execute_market_making(m_trading_data))
        {
            open_market_making_order(m_trading_data);
        }
    }
    
    // send data to client
    // if (m_tick_counter % 5 == 1)
    {
        Json send_data = prepare_sending_data_to_FE(m_trading_data);
        send_data_to_client_through_channel(CHANNEL_SCANNING_MARKET, send_data);
    }
}

bool MarketMakingTradingStrategy::can_execute_market_making(Json& calculated_data)
{
    bool ret = false;
    long price_precision = static_cast<long>(m_main_symbol_info["pricePrecision"]);
    if (m_is_arm_ask)
        m_theo_price = calculated_data["theo_ask"];
    else if (m_is_arm_bid)
        m_theo_price = calculated_data["theo_bid"];
    m_theo_price = Utils::instance().round_with_decimal(m_theo_price, price_precision);

    long double delta_price = m_theo_price_tick * (long double)m_main_symbol_info["tickSize"];
    delta_price += std::numeric_limits<long double>::epsilon();

    if (fabs(m_theo_price - m_last_price) > delta_price)
        ret = true;

    return ret;
}

bool MarketMakingTradingStrategy::open_market_making_order(Json& calculated_data)
{
    if (m_main_qty_left < m_minimum_main_order_size) return false;
    // if (IS_EQUAL(m_main_qty_left, m_main_qty_error)) return false;

    long qty_precision = static_cast<long>(m_main_symbol_info["qtyPrecision"]);
    long price_precision = static_cast<long>(m_main_symbol_info["pricePrecision"]);
    Json query_json = {
        {"side", m_is_arm_bid ? "BUY" : "SELL"},
        {"type", "LIMIT"},
        {"symbol", m_main_symbol},
        {"price", TO_STRING(m_theo_price, price_precision)},
        {"quantity", TO_STRING(m_main_qty_left, qty_precision)}
    };
    if (m_trading_market_id == BINANCE_BLVT)
    {
        query_json.remove_field("price");
        query_json.remove_field("type");
        query_json["symbol"] = (string&&)m_main_symbol_info["baseAsset"];
        if (m_is_arm_bid)
        {
            query_json.remove_field("quantity");
            query_json["amount"] = TO_STRING(m_main_qty_left * (long double)calculated_data["last_price"], qty_precision);
        }
    }

    m_client_order_id = ScanningStrategiesManager::instance().get_client_order_id(m_strategy_id);
    m_orders_placed_set.insert(m_client_order_id);

    if (!BinanceAssetManager::instance().check_asset_available(m_trading_market_id, query_json))
    {
        // send to client
        Json err_data;
        err_data["message"] = "Spot asset is not available for SELL: " + (string&&)query_json["symbol"];
        err_data["code"] = NOTIFICATION_STATE_TRADE_ERROR;
        send_data_to_client_through_channel(CHANNEL_SCANNING_MARKET_NOTIFICATION, 
                                            err_data);

        return false;
    }

    // make a limit order first
    if (m_opened_order_id == 0)
    {
        query_json["newClientOrderId"] = m_client_order_id;
        ADD_LOG("MarketMaking first order: " << query_json);
        Json response = 
            ExchangeGateWay::instance().create_order(m_trading_market_id, query_json);
        // open order successful
        if (response["error"] == false)
        {
            // spot & future market
            if (m_trading_market_id == BINANCE_SPOT || m_trading_market_id == BINANCE_FUTURES)
            {
                ADD_LOG("MarketMaking first order: true");
                m_last_price = m_theo_price;
                m_opened_order_id = response["data"]["orderId"];
            }
            else if (m_trading_market_id == BINANCE_BLVT)
            {
                if ((string&&)response["data"]["status"] == "S")
                {
                    // add trade history
                    BinanceAssetManager::instance().query_blvt_record(
                                            (string&&)response["data"]["side"],
                                            (long)response["data"]["orderId"],
                                            m_strategy_id);

                    long double currency_qty = stold((string&&)response["data"]["quantity"]);
                    currency_qty *= fabs(m_initial_multiply_ratio);
                    open_hedging_order(currency_qty);
                    finish_trading_order();
                }
                ADD_LOG("MarketMaking BLVT ok: " << response.get_string_value());
            }
            return true;
        }
        else
        {
            if (m_trading_market_id == BINANCE_SPOT || m_trading_market_id == BINANCE_FUTURES)
            {
                ADD_LOG("MarketMaking first order: false " << m_main_qty_left);
                m_main_qty_error = m_main_qty_left;
            }
            else if (m_trading_market_id == BINANCE_BLVT)
            {
                ADD_LOG("MarketMaking BLVT error: " << response.get_string_value());
                finish_trading_order();
            }
        }
    }
    // make a cancel and replace order
    else
    {
        query_json["cancelOrderId"] = m_opened_order_id;
        query_json["newClientOrderId"] = m_client_order_id;

        ADD_LOG("MarketMaking replace order: " << query_json.get_string_value());
        ADD_LOG("replace order open: " << m_main_qty_left);
        Json response = 
            ExchangeGateWay::instance().replace_order(m_trading_market_id, query_json);

        ADD_LOG("replace order response: " << response.get_string_value());

        // place order successful
        if (response["cancelResult"] == "SUCCESS" && response["newOrderResult"] == "SUCCESS")
        {
            ADD_LOG("MarketMaking replace order: true");
            m_last_price = m_theo_price;
            m_opened_order_id = response["newOrderResponse"]["orderId"];

            return true;
        }
        // failed to replace order: because of partially filled
        else 
        {
            // partially filled
            if (response["cancelResult"] == "SUCCESS" && response["newOrderResult"] != "SUCCESS")
            {
                ADD_LOG("MarketMaking replace order error: partially filled");
                m_main_qty_error = m_main_qty_left;
            }
            // filled immediately 
            else if (response["cancelResult"] != "SUCCESS")
            {
                ADD_LOG("MarketMaking replace order error: filled");
                // stop trading & wait next step from websocket if have
                m_strategy_inputs["request_type"] = "scan";
                m_trading_state = STATE_SCANNING;
            }
            m_opened_order_id = 0;
        }
    }

    return false;
}

void MarketMakingTradingStrategy::process_execution_report(Json& trade)
{
    // multithread safe
    // unique_lock lock(m_trade_mutex);
    unique_lock lock(m_base_strategy_on_tick_mutex);

    // check orderId in list of placed orders
    if (m_orders_placed_set.find((string&&)trade["clientOrderId"]) != m_orders_placed_set.end())
    {
        if ((string&&)trade["status"] == "PARTIALLY_FILLED" ||
            (string&&)trade["status"] == "FILLED")
        {
            ADD_LOG("MarketMakingTradingStrategy trade_callback: " << trade.get_string_value());
            // get cumulative quantity
            long qty_precision = static_cast<long>(m_currency_symbol_info["qtyPrecision"]);
            long double lot_size = static_cast<long double>(m_currency_symbol_info["lotSize"]);

            m_cumulative_main_qty += stold((string&&)trade["lastExecutedQuantity"]);
            m_main_qty_left = m_main_qty - m_cumulative_main_qty;

            long double cumulative_currency_qty = m_cumulative_main_qty * fabs(m_initial_multiply_ratio);
            cumulative_currency_qty = Utils::instance().round_with_decimal(cumulative_currency_qty, qty_precision);

            long double currency_qty_left = m_currency_qty - cumulative_currency_qty;

            long double delta_qty = cumulative_currency_qty - m_last_cumulative_currency_qty;
            delta_qty = Utils::instance().round_with_decimal(delta_qty, qty_precision);

            long ratio = round(delta_qty / lot_size);
            long min_ratio = round(m_minimum_currency_order_size / lot_size);

            ADD_LOG("execution_report m_cumulative_main_qty: " << m_cumulative_main_qty);
            ADD_LOG("execution_report m_main_qty_left: " << m_main_qty_left);
            // ADD_LOG("execution_report cumulative_currency_qty: " << cumulative_currency_qty);
            // ADD_LOG("execution_report delta_qty: " << delta_qty);
            // ADD_LOG("execution_report currency_qty_left: " << currency_qty_left);

            // need open a hedging order
            if (ratio > 0 && ratio >= min_ratio)
            if (currency_qty_left >= m_minimum_currency_order_size || 
                    currency_qty_left < lot_size * 0.9)
            {
                open_hedging_order(ratio * lot_size);
            }

            // all trade filled
            if ((string&&)trade["status"] == "FILLED")
            {
                finish_trading_order();
            }
        }
    }
}

void MarketMakingTradingStrategy::open_hedging_order(long double quantity)
{
    if ((string&&)(m_strategy_inputs["hedge_strategy"]) != FUTURES_HEDGING_STRATEGY_NAME ||
        !m_is_auto_hedge)
        return;

    ADD_LOG("open_hedging_order quantity: " << quantity);

    // create inputs
    Json inputs = m_strategy_inputs["hedge_config"].clone();

    long qty_precision = static_cast<long>(m_currency_symbol_info["qtyPrecision"]);
    // run futures hedging
    Json query_json = {
        {"type", "MARKET"},
        {"symbol", m_currency_symbol},
        {"quantity", TO_STRING(quantity, qty_precision)}
    };
    // "same" positionSide -> BTCDOWN
    if (inputs["direction"] == "same")
    {
        query_json["side"] = m_is_arm_bid ? "BUY" : "SELL";
        // query_json["positionSide"] = "LONG";
    }
    // "opposite" positionSide -> BTCUP
    else
    {
        query_json["side"] = m_is_arm_bid ? "SELL" : "BUY";
        // query_json["positionSide"] = "SHORT";
    }
    if (m_scanning_market_id == BINANCE_BLVT && (string&&)query_json["side"] == "BUY")
        query_json["quantity"] = TO_STRING(quantity * (long double)m_currency_order_book["p"], qty_precision);

    inputs["symbol"] = m_currency_symbol;
    inputs["exchange"] = m_scanning_exchange_name;
    inputs["query_json"] = query_json;
    inputs["id"] = m_strategy_id;

    shared_ptr<HedgingStrategy> hedging_strategy = make_shared<BFHedgingStrategy>(inputs);
    hedging_strategy->start();
    m_hedging_strategies.push_back(hedging_strategy);
    m_hedging_state = STATE_HEDGING;

    // update last cumulative quantity
    if (hedging_strategy->is_hedging_finished())
        m_last_cumulative_currency_qty += quantity;
}
    
void MarketMakingTradingStrategy::finish_trading_order()
{
    // reset trade params
    reset_trade_params();

    // update repeat times
    m_strategy_inputs["trade_config"]["repeat_times"] = --m_repeat_times;

    // update states
    if (m_repeat_times <= 0)
    {    
        m_orders_placed_set.clear();

        m_strategy_inputs["request_type"] = "scan";
        m_trading_state = STATE_SCANNING;
    }
    else
    {
        m_strategy_inputs["request_type"] = "trade";
        m_trading_state = STATE_TRADING;
    }

    m_opened_order_id = 0;

    // update to DB
    save_strategy_inputs_to_DB();
}

Json MarketMakingTradingStrategy::calculate_trading_data()
{
    Json data;

    // Leverage token price 
    long double main_bid_price = m_main_order_book["b"];
    long double main_ask_price = m_main_order_book["a"];

    // BTCUSDT 
    long double currency_bid_price = m_currency_order_book["b"];
    long double currency_ask_price = m_currency_order_book["a"];

    // Calculate order data
    long double theo_bid, theo_ask;
    if ((string&&)(m_strategy_inputs["trade_strategy"]) == BLVT_TRADING_STRATEGY_UP_NAME ||
        (string&&)(m_strategy_inputs["trade_strategy"]) == MARKET_MAKING_TRADING_STRATEGY_UP_NAME)
    {
        theo_bid = calculate_theorical_bid(currency_bid_price);
        theo_ask = calculate_theorical_ask(currency_ask_price);
    }
    else
    {
        theo_bid = calculate_theorical_ask(currency_ask_price);
        theo_ask = calculate_theorical_bid(currency_bid_price);
    }

    data["theo_bid"] = theo_bid + m_offset_bid;
    data["theo_ask"] = theo_ask + m_offset_ask;
    data["best_ask"]  = main_ask_price;
    data["best_bid"]  = main_bid_price;
    data["last_price"]  = m_main_order_book["p"];

    return data;
}

long double MarketMakingTradingStrategy::calculate_theorical_bid(
                                        const long double currency_bid_price)
{
    long double theo_bid = currency_bid_price * m_initial_multiply_ratio;
    theo_bid -= m_initial_offset;
    return theo_bid;
}

long double MarketMakingTradingStrategy::calculate_theorical_ask(
                                        const long double currency_ask_price)
{
    long double theo_ask = currency_ask_price * m_initial_multiply_ratio;
    theo_ask -= m_initial_offset;
    return theo_ask;
}

void MarketMakingTradingStrategy::read_strategy_inputs()
{    
    read_common_inputs();
    
    m_initial_multiply_ratio = stold((string&&)m_strategy_inputs["trade_config"]["multiply_ratio"]);
    m_initial_offset = stold((string&&)m_strategy_inputs["trade_config"]["offset"]);
    m_repeat_times = static_cast<long>(m_strategy_inputs["trade_config"]["repeat_times"]);

    m_theo_price_tick = static_cast<long>(m_strategy_inputs["trade_config"]["theo_price_tick"]);
    m_minimum_main_order_size = stold((string&&)m_strategy_inputs["trade_config"]["minimum_main_order_size"]);
    m_minimum_currency_order_size = stold((string&&)m_strategy_inputs["trade_config"]["minimum_currency_order_size"]);

    if (m_repeat_times > 0)
    {
        if ((string&&)m_strategy_inputs["request_type"] == "scan")
            m_trading_state = STATE_SCANNING;
        else if ((string&&)m_strategy_inputs["request_type"] == "trade")
            m_trading_state = STATE_TRADING;
    }
    else 
    {
        m_trading_state = STATE_SCANNING;
    }
}

void MarketMakingTradingStrategy::reset_trade_params()
{
    if (m_is_arm_ask)
        m_main_qty = m_ask_quantity;
    else if (m_is_arm_bid)
        m_main_qty = m_bid_quantity;

    int precision = static_cast<long>(m_currency_symbol_info["qtyPrecision"]);
    m_currency_qty = Utils::instance().round_with_decimal(m_main_qty * fabs(m_initial_multiply_ratio), precision);
    
    m_main_qty_left = m_main_qty;
    m_main_qty_error = 0;

    m_last_cumulative_currency_qty = 0;
    m_last_price = 0;
    m_theo_price = 0;
    m_cumulative_main_qty = 0;
}

void MarketMakingTradingStrategy::add_symbols_to_order_book()
{
    // init callbacks
    auto order_book_callback = [](const std::string& symbol, Json& payload)
        {
            OrderBookManager::instance().update_order_book(payload);            
        };

    // add asset to order book
    bool is_new_symbol = 
        OrderBookManager::instance().add_order_book(m_symbol_list[0]);
    if (is_new_symbol)
    {
        if (m_trading_market_id == BINANCE_SPOT || m_trading_market_id == BINANCE_FUTURES)
        {
            ExchangeGateWay::instance().subscribe_data(m_trading_market_id, BOOK_TICKER, 
                                        m_main_symbol, order_book_callback);
            ExchangeGateWay::instance().subscribe_data(m_trading_market_id, AGG_TRADE, 
                                        m_main_symbol, order_book_callback);
        }
        else
        {
            ExchangeGateWay::instance().subscribe_data(m_trading_market_id, K_LINE, 
                                        (string&&)m_main_symbol_info["baseAsset"], order_book_callback);
        }
    }
    // add scan_symbol to order book
    is_new_symbol = 
        OrderBookManager::instance().add_order_book(m_symbol_list[1]);
    if (is_new_symbol)
    {
        if (m_scanning_market_id == BINANCE_SPOT || m_scanning_market_id == BINANCE_FUTURES)
        {
            ExchangeGateWay::instance().subscribe_data(m_scanning_market_id, BOOK_TICKER, 
                                        m_currency_symbol, order_book_callback);
            ExchangeGateWay::instance().subscribe_data(m_scanning_market_id, AGG_TRADE, 
                                        m_currency_symbol, order_book_callback);
        }
        else
        {
            ExchangeGateWay::instance().subscribe_data(m_scanning_market_id, K_LINE, 
                                        (string&&)m_currency_symbol_info["baseAsset"], order_book_callback);
        }
    }
}

void MarketMakingTradingStrategy::remove_symbols_from_order_book()
{
    // remove asset from order book
    bool is_symbol_removed = 
        OrderBookManager::instance().remove_order_book(m_symbol_list[0]);
    if (is_symbol_removed)
    {
        if (m_trading_market_id == BINANCE_SPOT || m_trading_market_id == BINANCE_FUTURES)
        {
            ExchangeGateWay::instance().unsubscribe_data(m_trading_market_id, BOOK_TICKER, m_main_symbol);
            ExchangeGateWay::instance().unsubscribe_data(m_trading_market_id, AGG_TRADE, m_main_symbol);
        }
        else
        {
            ExchangeGateWay::instance().unsubscribe_data(m_trading_market_id, K_LINE, (string&&)m_main_symbol_info["baseAsset"]);
        }
    }
    // remove scan_symbol from order book
    is_symbol_removed = 
        OrderBookManager::instance().remove_order_book(m_symbol_list[1]);
    if (is_symbol_removed)
    {
        if (m_scanning_market_id == BINANCE_SPOT || m_scanning_market_id == BINANCE_FUTURES)
        {
            ExchangeGateWay::instance().unsubscribe_data(m_scanning_market_id, BOOK_TICKER, m_currency_symbol);
            ExchangeGateWay::instance().unsubscribe_data(m_scanning_market_id, AGG_TRADE, m_currency_symbol);
        }
        else
        {
            ExchangeGateWay::instance().unsubscribe_data(m_scanning_market_id, K_LINE, (string&&)m_currency_symbol_info["baseAsset"]);
        }
    }
}

ResponseStatusCode MarketMakingTradingStrategy::update_strategy_inputs(Json& strategy_inputs)
{
    unique_lock lock(m_base_strategy_on_tick_mutex);

    // unsubscribe old symbols
    unsubscribe_symbols();
 
    // if (m_hedging_strategies.size() > 0)
    //     return INTERNAL_SERVER_ERROR_500;

    // if (((string&&)m_strategy_inputs["asset"] != (string&&)strategy_inputs["asset"]) ||
    //     ((string&&)m_strategy_inputs["trade_config"]["scan_symbol"] != (string&&)strategy_inputs["trade_config"]["scan_symbol"]))
    //     return BAD_REQUEST_400;

    // update inputs
    m_strategy_inputs = strategy_inputs.clone();

    try
    {
        // read inputs
        // read_strategy_inputs();
        start();
    }
    catch(...)
    {
        return BAD_REQUEST_400;
    }
    
    // update to DB
    save_strategy_inputs_to_DB();

    return OK_200;
}

void MarketMakingTradingStrategy::stop_trading(bool force_stop_hedging)
{
    if (m_trading_state != STATE_DELETED)
    {
        // cancel the LIMIT trading order
        if (m_opened_order_id > 0)
            ExchangeGateWay::instance().cancel_order(BINANCE_SPOT, m_main_symbol, 
                                                    m_opened_order_id);

        if (force_stop_hedging)
        {
            // stop hedging immediately
            for(auto strategy : m_hedging_strategies)
            {
                strategy->force_stop();
            }
            m_hedging_strategies.clear();

            m_hedging_state = STATE_NONE;
        }

        m_trading_state = STATE_SCANNING;
        
        // update to DB
        m_strategy_inputs["request_type"] = "scan";
        save_strategy_inputs_to_DB();
    }
}
