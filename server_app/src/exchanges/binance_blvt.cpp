#include <utils.h>
#include <exchanges/binance_blvt.h>
#include <api_handler/api_handler_binance_blvt/api_handler_binance_blvt_redeem.h>
#include <api_handler/api_handler_binance_blvt/api_handler_binance_blvt_subscribe.h>
#include <api_handler/api_handler_binance_blvt/api_handler_binance_blvt_query_redemption.h>
#include <api_handler/api_handler_binance_blvt/api_handler_binance_blvt_query_subscription.h>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt_user.h>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt_info.h>
#include <data_feed/data_feed_binance_blvt/data_feed_binance_blvt_candlestick.h>

#include <app_constants.h>

using namespace std;

BinanceBLVT::BinanceBLVT()
{
    this->m_url = BINANCE_BLVT_URL;
    this->m_port = BINANCE_BLVT_PORT;
    this->m_ws_url = BINANCE_BLVT_WS_URL;
    this->m_ws_port = BINANCE_BLVT_WS_PORT;

    m_user_rate_limits["REQUEST_WEIGHT_1MIN"] = 0;
    m_user_rate_limits["ORDERS_10SEC"] = 0;
    m_user_rate_limits["ORDERS_1DAY"] = 0;
    m_user_rate_limits["RAW_REQUESTS"] = 0;
    m_rate_limits_time = time(NULL);
}

BinanceBLVT::~BinanceBLVT()
{
    ADD_LOG("~BinanceBLVT()");
}

void BinanceBLVT::initialize()
{
    ADD_LOG("BinanceBLVT initialize");
    Json exchange_info = APIHandlerBinanceBLVT(nullptr).set_url(this->m_url).send_binance_normal_request("/api/v3/exchangeInfo", "");

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
    ADD_LOG("BinanceBLVT RateLimits: " << m_rate_limits["REQUEST_WEIGHT_1MIN"] << ", " << m_rate_limits["ORDERS_1DAY"] << ", " << m_rate_limits["ORDERS_10SEC"]);

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

Json BinanceBLVT::get_exchange_info(const std::string& symbol)
{
    return m_exchange_info[symbol];
}

Json BinanceBLVT::get_symbol_list()
{
    return m_symbol_list;
}

Json BinanceBLVT::get_account_info()
{
    return Json();
}

Json BinanceBLVT::get_order(Json& query_json)
{
    Json res = this->rate_limit_checking(1);
    if (res["status_code"] == OK_200)
    {
        if (query_json.has_field("side"))
        {
            if (query_json["side"] == "BUY")
            {
                return APIHandlerBinanceBLVTQuerySubscripton::handle_internal_request(query_json);
            }
            else if (query_json["side"] == "SELL")
            {
                return APIHandlerBinanceBLVTQueryRedemption::handle_internal_request(query_json);
            }
        }
    }
    return res;
}

Json BinanceBLVT::get_open_order(const std::string& symbol)
{
    return Json();
}

Json BinanceBLVT::create_order(Json& query_json)
{
    Json res = this->rate_limit_checking(1, true);
    if (res["status_code"] == OK_200)
    {
        if (query_json.has_field("side"))
        {
            if (query_json["side"] == "BUY")
            {
                Json response =  APIHandlerBinanceBLVTSubscribe::handle_internal_request(query_json);
                m_user_feed->on_rest_response(response);    
                return response; 
            }
            else if (query_json["side"] == "SELL")
            {
                Json response = APIHandlerBinanceBLVTRedeem::handle_internal_request(query_json);
                m_user_feed->on_rest_response(response);
                return response; 
            }
        }
    }
    return res;
}

Json BinanceBLVT::replace_order(Json& query_json)
{
    return Json();
}

Json BinanceBLVT::cancel_order(const std::string& symbol, const long orderId)
{
    return Json();
}

bool BinanceBLVT::start_user_feed()
{
    m_user_feed = std::make_shared<DataFeedBinanceBLVTUser>("", "");
    m_user_feed->start();
    return true;
}

size_t BinanceBLVT::subscribe_user_feed(UserCallback callback)
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

void BinanceBLVT::unsubscribe_user_feed(size_t callback_id)
{
    m_user_feed->remove_call_back(callback_id);
}

void BinanceBLVT::start_kline_feed(const std::string& symbol, DataCallback callback)
{
    auto ws = std::make_shared<DataFeedBinanceBLVTCandlestick>(symbol);
    m_kline_feeds[symbol] = ws;
    ws->set_call_back(callback);
    ws->start();
}
void BinanceBLVT::start_market_info_feed(const std::string& symbol, DataCallback callback)
{
    auto ws = std::make_shared<DataFeedBinanceBLVTInfo>(symbol);
    m_info_feeds[symbol] = ws;
    ws->set_call_back(callback);
    ws->start();
}

bool BinanceBLVT::unsubscribe(MDSubscribeType type, const std::string& symbol)
{
    if (type == MARKET_INFO)
    {
        m_info_feeds.erase(symbol);
        return true;
    }
    else if (type == K_LINE)
    {
        m_kline_feeds.erase(symbol);
        return true;
    }

    return false;
}

void BinanceBLVT::rate_limit_sync(Json& header)
{
    // Thread safe
    static std::mutex blvt_rate_mutex;
    std::unique_lock lock(blvt_rate_mutex);

    if (header.has_field("x-mbx-used-weight-1m"))
        m_user_rate_limits["REQUEST_WEIGHT_1MIN"] = std::stoi(std::string(header["x-mbx-used-weight-1m"]));

    if (header.has_field("x-mbx-order-count-10s"))
        m_user_rate_limits["ORDERS_10SEC"] = std::stoi(std::string(header["x-mbx-order-count-10s"]));

    if (header.has_field("x-mbx-order-count-1d"))
        m_user_rate_limits["ORDERS_1DAY"] = std::stoi(std::string(header["x-mbx-order-count-1d"]));

    m_rate_limits_time = time(NULL);
    ADD_LOG("BinanceBLVT::rate_limit_sync: " << m_user_rate_limits["REQUEST_WEIGHT_1MIN"] << ", " << m_user_rate_limits["ORDERS_1DAY"] << ", " << m_user_rate_limits["ORDERS_10SEC"]);
}

Json BinanceBLVT::rate_limit_checking(const int request_weight, bool is_order)
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

    static std::mutex blvt_rate_mutex;
    std::unique_lock lock(blvt_rate_mutex);

    m_user_rate_limits["REQUEST_WEIGHT_1MIN"] += request_weight;
    if (is_order)
    {
        m_user_rate_limits["ORDERS_1DAY"] += 1;
        m_user_rate_limits["ORDERS_10SEC"] += 1;
    }
    m_rate_limits_time = current_time;
    
    return res;
}

time_t BinanceBLVT::m_rate_limits_time;
std::map<std::string, int>  BinanceBLVT::m_rate_limits;
std::map<std::string, int>  BinanceBLVT::m_user_rate_limits;