#include <utils.h>
#include <exchanges/binance_spot.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_exchange_info.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_account_info.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_get_order.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_get_open_orders.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_create_order.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_cancel_order.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_cancel_replace_order.h>
#include <data_feed/data_feed_binance_spot/data_feed_binance_user.h>
#include <data_feed/data_feed_binance_spot/data_feed_binance_depth.h>
#include <data_feed/data_feed_binance_spot/data_feed_binance_book_ticker.h>
#include <data_feed/data_feed_binance_spot/data_feed_binance_agg_trade.h>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt_info.h>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt_candlestick.h>

#include <app_constants.h>

using namespace std;

BinanceSpot::BinanceSpot()
{
    this->m_url = BINANCE_SPOT_URL;
    this->m_port = BINANCE_SPOT_PORT;
    this->m_ws_url = BINANCE_SPOT_WS_URL;
    this->m_ws_port = BINANCE_SPOT_WS_PORT;

    m_user_rate_limits["REQUEST_WEIGHT_1MIN"] = 0;
    m_user_rate_limits["ORDERS_10SEC"] = 0;
    m_user_rate_limits["ORDERS_1DAY"] = 0;
    m_user_rate_limits["RAW_REQUESTS"] = 0;
    m_rate_limits_time = time(NULL);
}

BinanceSpot::~BinanceSpot()
{
    ADD_LOG("~BinanceSpot()");
}

void BinanceSpot::initialize()
{
    ADD_LOG("BinanceSpot initialize");
    Json exchange_info = APIHandlerBinance(nullptr).set_url(BINANCE_SPOT_URL).send_binance_normal_request("/api/v3/exchangeInfo", "");
    //Json exhange_info = APIHandlerBinanceExchangeInfo().set_url(m_url).get_exchange_info();

    // Save rate limits
    Json rate_limits = exchange_info["rateLimits"];

    rate_limits.for_each([this](Json& limit_info)
    {
        if(limit_info["rateLimitType"] == "REQUEST_WEIGHT")
        {
            m_rate_limits["REQUEST_WEIGHT_1MIN"] = (int)limit_info["limit"];
        }
        else if(limit_info["rateLimitType"] == "ORDERS")
        {
            if(limit_info["interval"] == "SECOND")
            {
                m_rate_limits["ORDERS_10SEC"] = limit_info["limit"];
            }
            else if(limit_info["interval"] == "DAY")
            {
                m_rate_limits["ORDERS_1DAY"] = limit_info["limit"];
            }
        }
        else if(limit_info["rateLimitType"] == "RAW_REQUESTS")
        {
            m_rate_limits["RAW_REQUESTS"] = limit_info["limit"];
        }
    });
    ADD_LOG("BinanceSpot RateLimits: " << m_rate_limits["REQUEST_WEIGHT_1MIN"] << ", " << m_rate_limits["ORDERS_1DAY"] << ", " << m_rate_limits["ORDERS_10SEC"]);

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

Json BinanceSpot::get_exchange_info(const std::string& symbol)
{
    return m_exchange_info[symbol];
}

Json BinanceSpot::get_symbol_list()
{
    return m_symbol_list;
}

Json BinanceSpot::get_account_info()
{
    Json res = this->rate_limit_checking(20);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceAccountInfo::handle_internal_request();
    }
    return res;
}

Json BinanceSpot::get_order(Json& query_json)
{
    Json res = this->rate_limit_checking(4);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceGetOrder::handle_internal_request(std::string(query_json["symbol"]), std::stol(std::string(query_json["orderId"])));
    }
    return res;
}

Json BinanceSpot::get_open_order(const std::string& symbol)
{
    Json res = this->rate_limit_checking(6);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceGetOpenOrders::handle_internal_request(symbol);
    }
    return res;
}

Json BinanceSpot::create_order(Json& query_json)
{
    // Need to define limit in APIHandlerBinanceCreateOrder as static function
    Json res = this->rate_limit_checking(1, true);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceCreateOrder::handle_internal_request(query_json);
    }
    return res;
}

Json BinanceSpot::replace_order(Json& query_json)
{
    Json res = this->rate_limit_checking(1);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceCancelReplaceOrder::handle_internal_request(query_json);
    }
    return res;
}

Json BinanceSpot::cancel_order(const std::string& symbol, const long orderId)
{
    Json res = this->rate_limit_checking(1);
    if (res["status_code"] == OK_200)
    {
        return APIHandlerBinanceCancelOrder::handle_internal_request(symbol, orderId);
    }
    return res;
}

bool BinanceSpot::start_user_feed()
{
    Json user = MongoDB::instance()
        .set_db_and_collection(BINANCE_SPOT_DB_SOURCE_NAME, "info")
        .find_one("user_id", "root");

    if (user.is_null() == false)
    {
        string api_key = (std::string&&)user["api_key"];
        string api_secret = (std::string&&)user["api_secret"];
        ADD_LOG("start_user_feed: BinanceSpot" );
        m_user_feed = std::make_shared<DataFeedBinanceUser>(api_key, api_secret);
        m_user_feed->start();

        return true;
    }

    return false;
}

size_t BinanceSpot::subscribe_user_feed(UserCallback callback)
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

void BinanceSpot::unsubscribe_user_feed(size_t callback_id)
{
    m_user_feed->remove_call_back(callback_id);
}

void BinanceSpot::start_depth_feed(const std::string& symbol, DataCallback callback)
{
    auto ws = std::make_shared<DataFeedBinanceDepth>(symbol);
    m_depth_feeds[symbol] = ws;
    ws->set_call_back(callback);
    ws->start();
}

void BinanceSpot::start_book_ticker_feed(const std::string& symbol, DataCallback callback)
{
    auto ws = std::make_shared<DataFeedBinanceBookTicker>(symbol);
    m_ticker_feeds[symbol] = ws;
    ws->set_call_back(callback);
    ws->start();
}

void BinanceSpot::start_agg_trade_feed(const std::string& symbol, DataCallback callback)
{
    auto ws = std::make_shared<DataFeedBinanceAggTrade>(symbol);
    m_agg_trade_feeds[symbol] = ws;
    ws->set_call_back(callback);
    ws->start();
}

bool BinanceSpot::unsubscribe(MDSubscribeType type, const std::string& symbol)
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

void BinanceSpot::rate_limit_sync(Json& header)
{
    // Thread safe
    static std::mutex bs_rate_mutex;
    std::unique_lock lock(bs_rate_mutex);

    if (header.has_field("x-mbx-used-weight-1m"))
        m_user_rate_limits["REQUEST_WEIGHT_1MIN"] = std::stoi(std::string(header["x-mbx-used-weight-1m"]));

    if (header.has_field("x-mbx-order-count-10s"))
        m_user_rate_limits["ORDERS_10SEC"] = std::stoi(std::string(header["x-mbx-order-count-10s"]));

    if (header.has_field("x-mbx-order-count-1d"))
        m_user_rate_limits["ORDERS_1DAY"] = std::stoi(std::string(header["x-mbx-order-count-1d"]));

    m_rate_limits_time = time(NULL);
    ADD_LOG("BinanceSpot::rate_limit_sync: " << m_user_rate_limits["REQUEST_WEIGHT_1MIN"] << ", " << m_user_rate_limits["ORDERS_1DAY"] << ", " << m_user_rate_limits["ORDERS_10SEC"]);
}

Json BinanceSpot::rate_limit_checking(const int request_weight, bool is_order)
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
        m_user_rate_limits["REQUEST_WEIGHT_1MIN"] = 0;
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
        m_user_rate_limits["ORDERS_10SEC"] = 0;
    }

    if (is_order && (m_user_rate_limits["ORDERS_1DAY"] > m_rate_limits["ORDERS_1DAY"]*0.9))
    {
        if (current_time/86400 == m_rate_limits_time/86400)
        {
            ADD_LOG("RATE_LIMIT_ERROR: ORDERS_1DAY PREVENTION. " << m_user_rate_limits["ORDERS_1DAY"]);
            res["status_code"] = RISK_ERROR_410;
            res["msg"] = "RATE_LIMIT_ERROR: ORDERS_1DAY PREVENTION";
            res["error"] = true;
            return res;
        }
        m_user_rate_limits["ORDERS_1DAY"] = 0;
    }

    static std::mutex bs_rate_mutex;
    std::unique_lock lock(bs_rate_mutex);

    m_user_rate_limits["REQUEST_WEIGHT_1MIN"] += request_weight;
    if (is_order)
    {
        m_user_rate_limits["ORDERS_1DAY"] += 1;
        m_user_rate_limits["ORDERS_10SEC"] += 1;
    }
    m_rate_limits_time = current_time;

    return res;
}

time_t BinanceSpot::m_rate_limits_time;
std::map<std::string, int>  BinanceSpot::m_rate_limits;
std::map<std::string, int>  BinanceSpot::m_user_rate_limits;