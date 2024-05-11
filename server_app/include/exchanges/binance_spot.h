#ifndef BINANCE_SPOT_H
#define BINANCE_SPOT_H

#include <string>
#include <memory>

#include <json/json.h>
//#include <mongo_db/mongo_db.h>
#include <app_constants.h>
#include <exchanges/exchange.h>

class DataFeedBinanceUser;
class DataFeedBinanceDepth;
class DataFeedBinanceBookTicker;
class DataFeedBinanceAggTrade;

class BinanceSpot : public Exchange
{
public:
    BinanceSpot();
    ~BinanceSpot();

    //virtual std::string init_info();
    //virtual std::string verify_valid_source();

    virtual void initialize();
    virtual Json get_exchange_info(const std::string &);
    virtual Json get_account_info();
    virtual Json get_order(Json& query_json);
    virtual Json get_open_order(const std::string& symbol);

    virtual Json create_order(Json& query_json);
    virtual Json replace_order(Json& query_json);
    virtual Json cancel_order(const std::string& symbol, const long orderId);

    virtual bool start_user_feed();
    virtual size_t subscribe_user_feed(UserCallback callback);
    virtual void unsubscribe_user_feed(size_t callback_id);

    virtual void start_depth_feed(const std::string&, DataCallback);
    virtual void start_book_ticker_feed(const std::string&, DataCallback);
    virtual void start_agg_trade_feed(const std::string&, DataCallback);
    virtual void start_kline_feed(const std::string& symbol, DataCallback)          {}
    virtual void start_market_info_feed(const std::string& symbol, DataCallback)    {}

    virtual bool unsubscribe(MDSubscribeType, const std::string&);

    Json get_symbol_list();
    static void rate_limit_sync(Json& header);

protected:
    std::string m_url;
    std::string m_port;
    std::string m_ws_url;
    std::string m_ws_port;

private:
    Json rate_limit_checking(const int request_weight, bool is_order = false);

    Json m_exchange_info;
    Json m_symbol_list;
    
    static time_t  m_rate_limits_time;
    static std::map<std::string, int>     m_rate_limits;
    static std::map<std::string, int>     m_user_rate_limits;

    std::shared_ptr<DataFeedBinanceUser>     m_user_feed;
    std::map<std::string, std::shared_ptr<DataFeedBinanceDepth>>        m_depth_feeds;
    std::map<std::string, std::shared_ptr<DataFeedBinanceBookTicker>>   m_ticker_feeds;
    std::map<std::string, std::shared_ptr<DataFeedBinanceAggTrade>>     m_agg_trade_feeds;
};

#endif //BINANCE_SPOT_H