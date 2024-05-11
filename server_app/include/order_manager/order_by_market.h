#ifndef ORDER_BY_MARKET_H
#define ORDER_BY_MARKET_H

#include <vector>
#include <unordered_map>

#include <data_model/data_model.h>

class OrderByMarket
{
public:
    OrderByMarket() = delete;
    OrderByMarket(const std::string& db_name);

    void add_order(Json& order);
    void update_order(Json& order);
    
    Json get_order_list(const std::string& market, const std::string& db_name, long from, long to);
    Json get_order_by_id(long order_id);

protected:
    long m_from = 0;
    long m_to   = 0;
    std::string m_db_name;

    std::vector<DataModel> m_order_list;
    std::unordered_map<std::string, DataModel> m_order_map;

private:
    Json transform_to_json_list();
    std::string get_string_order_id(Json& order); 
};

#endif //ORDER_BY_MARKET_H