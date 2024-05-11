#include <strategy_engine/trading_strategy/blvt_scanning_trading_strategy.h>
#include <strategy_engine/hedging_strategy/bs_trailing_stop_hedging_strategy.h>
#include <strategy_engine/hedging_strategy/bf_hedging_strategy.h>
#include <strategy_engine/scanning_strategies_manager.h>
#include <strategy_engine/base_strategy.h>

#include <data_feed/data_processor/data_storage/order_book_manager.h>
#include <data_feed/data_processor/data_storage/order_book.h>

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

BLVTScanningTradingStrategy::BLVTScanningTradingStrategy(const Json inputs) :
    TradingStrategy(inputs)
{
    m_trading_state = STATE_SCANNING;
    m_hedging_state = STATE_NONE;
}

BLVTScanningTradingStrategy::~BLVTScanningTradingStrategy()
{
}

// init strategy, init event handler.
void BLVTScanningTradingStrategy::on_init()
{
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

    // get data model
    init_config_data_model();

    // tracking blvt info
    ScanBLVTInfo::instance().subscribe_symbol(m_main_symbol_info["baseAsset"]);

    // add tracking order's status - Exchange Gateway
    // m_order_manager_callback_id = 
    //     ExchangeGateWay::instance().subscribe_user_feed(m_trading_market_id, [this](Json& order)
    // {
    //     ADD_LOG("order_manager_callback: " << order.get_string_value());
    //     // if ((string&&)order["clientOrderId"] == m_client_order_id &&
    //     //     (string&&)order["status"] == "FILLED")
    //     // {
    //     //     ADD_LOG("subscribe_user_feed_callback: FILLED - " << order.get_string_value());
    //     //     order_filled_callback(stold(string(order["price"])));
    //     // }
    // });
}

// function is called during deinitialization and is the deinit event handler. 
void BLVTScanningTradingStrategy::on_deinit() 
{
    remove_symbols_from_order_book();

    // untracking blvt info
    ScanBLVTInfo::instance().unsubscribe_symbol(m_main_symbol_info["baseAsset"]);

    // Remove tracking order's status
    // if (m_order_manager_callback_id != -1)
    // {
    //     ExchangeGateWay::instance().unsubscribe_user_feed(m_trading_market_id, m_order_manager_callback_id);
    // }

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
void BLVTScanningTradingStrategy::on_tick() 
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
    {
        if (hit_expected_profit("bid", m_trading_data))
        {
            open_order_and_hedge("bid", m_trading_data);
        }
        if (hit_expected_profit("ask", m_trading_data))
        {
            open_order_and_hedge("ask", m_trading_data);
        }        
    }
    
    // send data to client
    // if (m_tick_counter % 5 == 1)
    {
        Json send_data = prepare_sending_data_to_FE(m_trading_data);
        send_data_to_client_through_channel(CHANNEL_SCANNING_MARKET, send_data);
    }
}

void BLVTScanningTradingStrategy::open_order_and_hedge(const string& type, Json& data)
{
    // BUY using bid_qty
    // SELL using ask_qty
    string leverage_quantity = string((string&&)m_strategy_inputs["bid_qty"]);
    if (type == "ask")
        leverage_quantity = string((string&&)m_strategy_inputs["ask_qty"]);

    long double leverage_price = m_main_order_book["a"];
    if (leverage_price < 0) return;

    long price_precision = static_cast<long>(m_main_symbol_info["pricePrecision"]);
    Json query_json = {
        {"side", type == "bid" ? "BUY" : "SELL"},
        {"symbol", m_main_symbol},
        {"price", TO_STRING(leverage_price, price_precision)},
        {"quantity", leverage_quantity}
    };
    if (m_trading_market_id == BINANCE_BLVT)
    {
        query_json.remove_field("price");
        query_json.remove_field("type");
        query_json["symbol"] = (string&&)m_main_symbol_info["baseAsset"];
        if (type == "bid")
        {
            long qty_precision = static_cast<long>(m_main_symbol_info["qtyPrecision"]);
            query_json.remove_field("quantity");
            long double amount = stold(leverage_quantity);
            amount *= (long double)m_main_order_book["b"];
            query_json["amount"] = TO_STRING(amount, qty_precision);
        }
    }

    // open leverage spot order
    open_order(query_json);

    // save to DB
    save_strategy_inputs_to_DB();
}

Json BLVTScanningTradingStrategy::create_spot_hedging_inputs(Json& order)
{
    Json query_json = order.clone();
    // fix side to SELL
    query_json["side"] = "SELL";
    query_json["type"] = "STOP_LOSS_LIMIT";
    
    // prepare inputs for strategy
    Json inputs = m_strategy_inputs["hedge_config"].clone();
    inputs["symbol"] = m_main_symbol;
    inputs["exchange"] = BINANCE_SPOT_ABBREVIATION_NAME;
    inputs["tick_size"] = m_main_symbol_info["tickSize"];
    inputs["price_precision"] = static_cast<long>(m_currency_symbol_info["pricePrecision"]);
    inputs["query_json"] = query_json;
    inputs["id"] = m_strategy_id;

    return inputs;
}

Json BLVTScanningTradingStrategy::create_future_hedging_inputs(Json& order)
{
    Json inputs = m_strategy_inputs["hedge_config"].clone();

    Json query_json = order.clone();
    query_json["symbol"] = m_currency_symbol;
    query_json["type"] = "MARKET";
    query_json.remove_field("price");
    
    string strategy_side = string((string&&)query_json["side"]);
    // same positionSide -> BTCDOWN
    if (inputs["direction"] == "same")
    {
        query_json["side"] = strategy_side == "BUY" ? "BUY" : "SELL";
        // query_json["positionSide"] = "LONG";
    }
    // opposite positionSide -> BTCUP
    else
    {
        query_json["side"] = strategy_side == "BUY" ? "SELL" : "BUY";
        // query_json["positionSide"] = "SHORT";
    }

    // calculate quantity
    long qty_precision = static_cast<long>(m_currency_symbol_info["qtyPrecision"]);
    long double leverage_quantity = m_bid_quantity * fabs(m_initial_multiply_ratio);
    if (m_scanning_market_id == BINANCE_BLVT)
        if ((string&&)query_json["side"] == "BUY")
            leverage_quantity *= (long double)m_currency_order_book["p"];
    query_json["quantity"] = TO_STRING(leverage_quantity, qty_precision);

    // prepare inputs for strategy
    inputs["symbol"] = m_currency_symbol;
    inputs["exchange"] = m_scanning_exchange_name;
    inputs["tick_size"] = m_currency_symbol_info["tickSize"];
    inputs["query_json"] = query_json;
    inputs["id"] = m_strategy_id;

    return inputs;
}

void BLVTScanningTradingStrategy::open_order(Json& query_json)
{
    ADD_LOG("open_order: " << query_json.get_string_value());
    Json hedging_inputs;
    if ((string&&)(m_strategy_inputs["hedge_strategy"]) == MOMENTUM_HEDGING_STRATEGY_NAME)
        hedging_inputs = create_spot_hedging_inputs(query_json);
    else
        hedging_inputs = create_future_hedging_inputs(query_json);

    query_json["type"] = "MARKET";
    query_json.remove_field("price");

    if (!BinanceAssetManager::instance().check_asset_available(m_trading_market_id, query_json))
    {
        // send to client
        Json err_data;
        err_data["message"] = "Spot asset is not available for SELL: " + (string&&)query_json["symbol"];
        err_data["code"] = NOTIFICATION_STATE_TRADE_ERROR;
        send_data_to_client_through_channel(CHANNEL_SCANNING_MARKET_NOTIFICATION, 
                                            err_data);

        return;
    }

    // send order to exchange
    m_client_order_id = ScanningStrategiesManager::instance().get_client_order_id(m_strategy_id);
    query_json["newClientOrderId"] = m_client_order_id;

    Json response = 
        ExchangeGateWay::instance().create_order(m_trading_market_id, query_json);

    // open order successful
    if (response["error"] == false)
    {
        ADD_LOG("open_order of scanning market success, reponse = " << response);
        // add trade history
        if (m_trading_market_id == BINANCE_BLVT)
        {
            BinanceAssetManager::instance().query_blvt_record(
                                    (string&&)response["data"]["side"],
                                    (long)response["data"]["orderId"],
                                    m_strategy_id);
        }
        // create hedging strategy
        if (m_is_auto_hedge)
        {
            shared_ptr<HedgingStrategy> hedging_strategy;
            if ((string&&)(m_strategy_inputs["hedge_strategy"]) == MOMENTUM_HEDGING_STRATEGY_NAME)
                hedging_strategy = make_shared<BSTrailingStopHedgingStrategy>(hedging_inputs);
            else
                hedging_strategy = make_shared<BFHedgingStrategy>(hedging_inputs);

            hedging_strategy->start();
            m_hedging_strategies.push_back(hedging_strategy);

            m_hedging_state = STATE_HEDGING;
        }

        // update repeat times
        m_strategy_inputs["trade_config"]["repeat_times"] = --m_repeat_times;

        // update states
        if (m_repeat_times <= 0)
            m_trading_state = STATE_SCANNING;
        else
            m_trading_state = STATE_TRADING;
    }
    // open order failed
    else 
    {
        ADD_LOG("open_order of scanning market error, reponse = " << response);
        // stop trading
        m_trading_state = STATE_NONE;
        m_repeat_times = 0;

        // inform to client
        Json err_data;
        err_data["message"] = response["msg"];
        err_data["code"] = NOTIFICATION_STATE_TRADE_ERROR;
        send_data_to_client_through_channel(CHANNEL_SCANNING_MARKET_NOTIFICATION, 
                                            err_data);
    }
}

void BLVTScanningTradingStrategy::order_filled_callback(long double price)
{
    // update filled price 
    // static_cast<BLVTScanningHedgingStrategy*>(m_hedging_strategy)->set_filled_price(price);

}

bool BLVTScanningTradingStrategy::hit_expected_profit(const string& type, Json& data)
{
    long double diff = -1;
    // check ARM condition
    if (type == "bid" && m_is_arm_bid)
    {
        diff = (long double)(data["theo_bid"]) - (long double)(data["best_ask"]);

        if (diff > 0 && m_repeat_times > 0)
        {
            return true;
        }
    }
    // check ARM condition
    else if (type == "ask" && m_is_arm_ask)
    if ((string&&)m_strategy_inputs["hedge_strategy"] == FUTURES_HEDGING_STRATEGY_NAME)
    {
        diff = (long double)(data["best_bid"]) - (long double)(data["theo_ask"]);

        if (diff > 0 && m_repeat_times > 0)
        {
            return true;
        }
    }

    return false;
}

Json BLVTScanningTradingStrategy::calculate_trading_data()
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

long double BLVTScanningTradingStrategy::calculate_theorical_bid(
                                        const long double currency_bid_price)
{
    long double theo_bid = currency_bid_price * m_initial_multiply_ratio;
    theo_bid -= m_initial_offset;
    return theo_bid;
}

long double BLVTScanningTradingStrategy::calculate_theorical_ask(
                                        const long double currency_ask_price)
{
    long double theo_ask = currency_ask_price * m_initial_multiply_ratio;
    theo_ask -= m_initial_offset;
    return theo_ask;
}

void BLVTScanningTradingStrategy::read_strategy_inputs()
{
    read_common_inputs();

    m_initial_multiply_ratio = stold((string&&)m_strategy_inputs["trade_config"]["multiply_ratio"]);
    m_initial_offset = stold((string&&)m_strategy_inputs["trade_config"]["offset"]);
    m_repeat_times = static_cast<long>(m_strategy_inputs["trade_config"]["repeat_times"]);

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

void BLVTScanningTradingStrategy::add_symbols_to_order_book()
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

void BLVTScanningTradingStrategy::remove_symbols_from_order_book()
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

ResponseStatusCode BLVTScanningTradingStrategy::update_strategy_inputs(Json& strategy_inputs)
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

void BLVTScanningTradingStrategy::stop_trading(bool force_stop_hedging)
{
    if (m_trading_state != STATE_DELETED)
    {
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
