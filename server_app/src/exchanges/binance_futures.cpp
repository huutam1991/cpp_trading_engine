#include <utils.h>
#include <exchanges/binance_futures.h>
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_exchange_info.h>
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_account_info.h>
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_get_order.h>
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_get_open_orders.h>
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_create_order.h>
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_cancel_order.h>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures_user.h>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures_depth.h>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures_book_ticker.h>
#include <data_feed/data_feed_binance_futures/data_feed_binance_futures_agg_trade.h>

#include <app_constants.h>

using namespace std;

BinanceFutures::BinanceFutures()
{
    this->m_url = BINANCE_FUTURES_URL;
    this->m_port = BINANCE_FUTURES_PORT;
    this->m_ws_url = BINANCE_FUTURES_WS_URL;
    this->m_ws_port = BINANCE_FUTURES_WS_PORT;

    m_user_rate_limits["REQUEST_WEIGHT_1MIN"] = 0;
    m_user_rate_limits["ORDERS_10SEC"] = 0;
    m_user_rate_limits["ORDERS_1MIN"] = 0;
    m_user_rate_limits["RAW_REQUESTS"] = 0;
    m_rate_limits_time = time(NULL);
}

BinanceFutures::~BinanceFutures()
{
    ADD_LOG("~BinanceFutures()");
}

void BinanceFutures::initialize()
{
    ADD_LOG("BinanceFutures initialize");
    Json exchange_info = APIHandlerBinanceFutures(nullptr).set_url(BINANCE_FUTURES_URL).send_binance_normal_request("/fapi/v1/exchangeInfo", "");
    //Json exchange_info = APIHandlerBinanceFuturesExchangeInfo().set_url(BINANCE_FUTURES_URL).get_exchange_info();

    // Save rate limits
    Json rate_limits = exchange_info["rateLimits"];
    rate_limits.for_each([this](Json& limit_info)
    {
        if(limit_info["rateLimitType"] == "REQUEST_WEIGHT")
        {
            m_rate_limits["REQUEST_WEIGHT_1MIN"] = limit_info["limit"];
        }
        else if(limit_info["rateLimitType"] == "ORDERS")
        {
            if(limit_info["interval"] == "SECOND")
            {
                m_rate_limits["ORDERS_10SEC"] = limit_info["limit"];
            }
            else if(limit_info["interval"] == "MINUTE")
            {
                m_rate_limits["ORDERS_1MIN"] = limit_info["limit"];
            }
        }
        else if(limit_info["rateLimitType"] == "RAW_REQUESTS")
        {
            m_rate_limits["RAW_REQUESTS"] = limit_info["limit"];
        }
    });
    ADD_LOG("BinanceFutures RateLimits: " << m_rate_limits["REQUEST_WEIGHT_1MIN"] << ", " << m_rate_limits["ORDERS_1MIN"] << ", " << m_rate_limits["ORDERS_10SEC"]);

    // Save symbol filters
    Json symbols = exchange_info["symbols"];
    m_symbol_list = Json::create_array();

    symbols.for_each([this](Json& symbol_info)
    {
        std::string symbol_name = symbol_info["symbol"];
        m_symbol_list.push_back(symbol_name);

        m_exchange_info[symbol_name]["symbol"] = symbol_name;
        m_exchange_info[symbol_name]["baseAsset"] = symbol_info["baseAsset"];
        m_exchange_info[symbol_name]["quoteAsset"] = symbol_info["quoteAsset"];

        Json& filter_list = symbol_info["filters"];
        filter_list.for_each([this, &symbol_name](Json& filter)
        {
            if(filter["filterType"] == "PRICE_FILTER")
            {
                m_exchange_info[symbol_name]["tickSize"] = std::stold((std::string&&)filter["tickSize"]);
                m_exchange_info[symbol_name]["pricePrecision"] = Utils::instance().get_decimal_digits(filter["tickSize"]);
            }
                
            else if(filter["filterType"] == "LOT_SIZE")
            {
                m_exchange_info[symbol_name]["lotSize"] = std::stold((std::string&&)filter["stepSize"]);
                m_exchange_info[symbol_name]["qtyPrecision"] = Utils::instance().get_decimal_digits(filter["stepSize"]);
            }
        });      
    });

    // get trade info
    start_user_feed();
}

Json BinanceFutures::get_exchange_info(const std::string& symbol)
{
    return m_exchange_info[symbol];
}

Json BinanceFutures::get_symbol_list()
{
    return m_symbol_list;
}

Json BinanceFutures::get_account_info()
{
    Json res = this->rate_limit_checking(5);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceFuturesAccountInfo::handle_internal_request();
    }
    return res;
}

Json BinanceFutures::get_order(Json& query_json)
{
    Json res = this->rate_limit_checking(1);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceFuturesGetOrder::handle_internal_request(std::string(query_json["symbol"]), std::stol(std::string(query_json["orderId"])));
    }
    return res;
}

Json BinanceFutures::get_open_order(const std::string& symbol)
{
    Json res = this->rate_limit_checking(1);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceFuturesGetOpenOrders::handle_internal_request(symbol);
    }
    return res;
}

Json BinanceFutures::create_order(Json& query_json)
{
    Json res = this->rate_limit_checking(0, true);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceFuturesCreateOrder::handle_internal_request(query_json);
    }
    return res;
}

Json BinanceFutures::replace_order(Json& query_json)
{
    return Json();
}

Json BinanceFutures::cancel_order(const std::string& symbol, const long orderId)
{
    Json res = this->rate_limit_checking(1);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceFuturesCancelOrder::handle_internal_request(symbol, orderId);
    }
    return res;
}

bool BinanceFutures::start_user_feed()
{
    Json user = MongoDB::instance()
        .set_db_and_collection(BINANCE_FUTURES_DB_SOURCE_NAME, "info")
        .find_one("user_id", "root");

    if (user.is_null() == false)
    {
        string api_key = (std::string&&)user["api_key"];
        string api_secret = (std::string&&)user["api_secret"];
        ADD_LOG("start_user_feed: BinanceFutures" );
        m_user_feed = std::make_shared<DataFeedBinanceFuturesUser>(api_key, api_secret);
        m_user_feed->start();

        return true;
    }

    return false;
}

size_t BinanceFutures::subscribe_user_feed(UserCallback callback)
{
    if (m_user_feed)
    {
        return m_user_feed->add_call_back(callback);
    }
    else
    {
        if (start_user_feed())
            return m_user_feed->add_call_back(callback);
        else
        {
            ADD_LOG("subscribe_user_feed, NO_USER_FEED");
            return 0;
        }
    } 
}

void BinanceFutures::unsubscribe_user_feed(size_t callback_id)
{
    m_user_feed->remove_call_back(callback_id);
}

void BinanceFutures::start_depth_feed(const std::string& symbol, DataCallback callback)
{
    auto ws = std::make_shared<DataFeedBinanceFuturesDepth>(symbol);
    m_depth_feeds[symbol] = ws;
    ws->set_call_back(callback);
    ws->start();
}

void BinanceFutures::start_book_ticker_feed(const std::string& symbol, DataCallback callback)
{
    auto ws = std::make_shared<DataFeedBinanceFuturesBookTicker>(symbol);
    m_ticker_feeds[symbol] = ws;
    ws->set_call_back(callback);
    ws->start();
}

void BinanceFutures::start_agg_trade_feed(const std::string& symbol, DataCallback callback)
{
    auto ws = std::make_shared<DataFeedBinanceFuturesAggTrade>(symbol);
    m_agg_trade_feeds[symbol] = ws;
    ws->set_call_back(callback);
    ws->start();
}

bool BinanceFutures::unsubscribe(MDSubscribeType type, const std::string& symbol)
{
    if (type == DEPTH)
    {
        m_depth_feeds.erase(symbol);
        return true;
    }
    else if (type == BOOK_TICKER)
    {
        m_ticker_feeds.erase(symbol);
        return true;
    }
    else if (type == AGG_TRADE)
    {
        m_agg_trade_feeds.erase(symbol);
        return true;
    }

    return false;
}

void BinanceFutures::rate_limit_sync(Json& header)
{
    // Thread safe
    static std::mutex bf_rate_mutex;
    std::unique_lock lock(bf_rate_mutex);

    if (header.has_field("x-mbx-used-weight-1m"))
        m_user_rate_limits["REQUEST_WEIGHT_1MIN"] = std::stoi(std::string(header["x-mbx-used-weight-1m"]));

    if (header.has_field("x-mbx-order-count-10s"))
        m_user_rate_limits["ORDERS_10SEC"] = std::stoi(std::string(header["x-mbx-order-count-10s"]));

    if (header.has_field("x-mbx-order-count-1m"))
        m_user_rate_limits["ORDERS_1MIN"] = std::stoi(std::string(header["x-mbx-order-count-1m"]));

    m_rate_limits_time = time(NULL);
    ADD_LOG("BinanceFutures::rate_limit_sync: " << m_user_rate_limits["REQUEST_WEIGHT_1MIN"] << ", " << m_user_rate_limits["ORDERS_1MIN"] << ", " << m_user_rate_limits["ORDERS_10SEC"]);
}

Json BinanceFutures::rate_limit_checking(const int request_weight, bool is_order)
{
    //Move this handle to RiskManager
    Json res;
    res["error"] = false;
    res["status_code"] = OK_200;

    time_t current_time = time(NULL);
    if (m_user_rate_limits["REQUEST_WEIGHT_1MIN"] > m_rate_limits["REQUEST_WEIGHT_1MIN"]*0.9)
    {
        if (current_time/60 == m_rate_limits_time/60)
        {
            ADD_LOG("RATE_LIMIT_ERROR: REQUEST_WEIGHT_1MIN PREVENTION. " << m_user_rate_limits["REQUEST_WEIGHT_1MIN"]);
            res["status_code"] = RISK_ERROR_410;
            res["msg"] = "RATE_LIMIT_ERROR: REQUEST_WEIGHT_1MIN PREVENTION";
            res["error"] = true;
            return res;
        }
        this->m_user_rate_limits["REQUEST_WEIGHT_1MIN"] = 0;
    }
    
    if (is_order && (m_user_rate_limits["ORDERS_10SEC"] > m_rate_limits["ORDERS_10SEC"]*0.9))
    {
        if (current_time/10 == m_rate_limits_time/10)
        {
            ADD_LOG("RATE_LIMIT_ERROR: ORDERS_10SEC PREVENTION. " << m_user_rate_limits["ORDERS_10SEC"]);
            res["status_code"] = RISK_ERROR_410;
            res["msg"] = "RATE_LIMIT_ERROR: ORDERS_10SEC PREVENTION";
            res["error"] = true;
            return res;
        }
        this->m_user_rate_limits["ORDERS_10SEC"] = 0;
    }

    if (is_order && (m_user_rate_limits["ORDERS_1MIN"] > m_rate_limits["ORDERS_1MIN"]*0.9))
    {
        if (current_time/60 == m_rate_limits_time/60)
        {
            ADD_LOG("RATE_LIMIT_ERROR: ORDERS_1MIN PREVENTION. " << m_user_rate_limits["ORDERS_1MIN"]);
            res["status_code"] = RISK_ERROR_410;
            res["msg"] = "RATE_LIMIT_ERROR: ORDERS_1MIN PREVENTION";
            res["error"] = true;
            return res;
        }
        this->m_user_rate_limits["ORDERS_1MIN"] = 0;
    }

    static std::mutex bf_rate_mutex;
    std::unique_lock lock(bf_rate_mutex);

    m_user_rate_limits["REQUEST_WEIGHT_1MIN"] += request_weight;
    if (is_order)
    {
        m_user_rate_limits["ORDERS_1MIN"] += 1;
        m_user_rate_limits["ORDERS_10SEC"] += 1;
    }
    m_rate_limits_time = time(NULL);
    
    return res;
}

time_t BinanceFutures::m_rate_limits_time;
std::map<std::string, int>  BinanceFutures::m_rate_limits;
std::map<std::string, int>  BinanceFutures::m_user_rate_limits;