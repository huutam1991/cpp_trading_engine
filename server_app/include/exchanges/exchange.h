#ifndef EXCHANGE_H
#define EXCHANGE_H

#include <string>
#include <functional>
//#include <memory>
//#include <unordered_map>
//#include <mongo_db/mongo_db.h>
#include <app_constants.h>
#include <json/json.h>
#include <storage_source/storage_source.h>

using UserCallback = std::function<void(Json& payload)>;
using DataCallback = std::function<void(const std::string& symbol, Json& payload)>;

class Exchange
{
public:
    Exchange();
    virtual ~Exchange(); 

    virtual void initialize()    = 0;
    virtual Json get_exchange_info(const std::string& symbol)                   = 0;
    virtual Json get_account_info()                                             = 0;
    virtual Json get_order(Json& query_json)                                    = 0;
    virtual Json get_open_order(const std::string& symbol)                      = 0;
    virtual Json create_order(Json& query_json)                                 = 0;
    virtual Json replace_order(Json& query_json)                                = 0;
    virtual Json cancel_order(const std::string& symbol, const long orderId)    = 0;

    virtual bool start_user_feed()   = 0;
    virtual size_t subscribe_user_feed(UserCallback callback)   = 0;
    virtual void unsubscribe_user_feed(size_t callback_id)      = 0;

    virtual void start_depth_feed(const std::string& symbol, DataCallback callback)        = 0;
    virtual void start_agg_trade_feed(const std::string& symbol, DataCallback callback)    = 0;
    virtual void start_book_ticker_feed(const std::string& symbol, DataCallback callback)  = 0;
    virtual void start_kline_feed(const std::string& symbol, DataCallback callback)        = 0;
    virtual void start_market_info_feed(const std::string& symbol, DataCallback callback)  = 0;

    virtual bool unsubscribe(MDSubscribeType type, const std::string& symbol)   = 0;

protected:
    std::string m_url;
    std::string m_port;
    std::string m_ws_url;
    std::string m_ws_port;
};

#endif //EXCHANGE_H