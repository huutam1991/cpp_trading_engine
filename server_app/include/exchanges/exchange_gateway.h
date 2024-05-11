#ifndef EXCHANGE_GATEWAY_H
#define EXCHANGE_GATEWAY_H

#include <string>
//#include <memory>
//#include <mongo_db/mongo_db.h>
#include <app_constants.h>
#include <util_macros.h>
#include <json/json.h>

#include <exchanges/exchange.h>

class ExchangeGateWay
{
    Singleton(ExchangeGateWay);

public:
    void initialize();

    Json get_exchange_info(Market market, const std::string& symbol);
    Json get_account_info(Market market);
    Json get_order(Market market, Json& query_json);
    Json get_open_order(Market market, const std::string& symbol);

    Json create_order(Market market, Json& query_json);
    Json replace_order(Market market, Json& query_json);
    Json cancel_order(Market market, const std::string& symbol, const long orderId);

    void start_user_feed(Market market);
    size_t subscribe_user_feed(Market market, UserCallback);
    void unsubscribe_user_feed(Market market, size_t callback_id);
    
    size_t subscribe_data(Market market, MDSubscribeType type, const std::string& symbol, DataCallback callback);
    void unsubscribe_data(Market market, MDSubscribeType type, const std::string& symbol);

private:
    std::map<Market, Exchange*>    m_ExchangeList;
};

#endif //EXCHANGE_GATEWAY_H