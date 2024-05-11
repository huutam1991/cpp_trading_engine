#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <mutex>
#include <functional>
#include <memory>

#include <util_macros.h>
#include <json/json.h>
#include <order_manager/order_by_market.h>

class OrderManager
{
    Singleton(OrderManager);

public:
    Json get_order(const std::string& ex_name, const std::string& db_name, long order_id);
    Json get_order_list(const std::string& ex_name, const std::string& db_name, long from, long to);
    
    size_t subscribe_user_data(std::function<void(Json&)> callback);  
    void unsubscribe_user_data(size_t callback_id);

    //void set_callback_by_order_id(const std::string& symbol, size_t order_id, std::function<void(Json&)> callback);
    //void remove_callback_by_order_id(const std::string& symbol, size_t order_id);

    void update_order(const std::string& ex_name, Json& order);
    void clean_all_cached_orders();

protected:  
    //void format_order(Json& order);
    //void copy_order(Json& order, Json& copy_order);

    void invoke_callback(Json& order);
    std::string get_string_order_id(Json& order);

    std::unique_ptr<OrderByMarket>& get_order_by_user(const std::string& user_id, const std::string& db_name);
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<OrderByMarket>>> m_order_by_user_list;

private:
    size_t m_callback_id = 0;
    std::mutex m_order_manager_mutex;

    std::unordered_map<size_t, std::function<void(Json&)>>      m_subscribed_callback_list;
    //std::unordered_map<std::string, std::function<void(Json&)>> m_callback_list_by_order_id;
};

#endif //ORDER_MANAGER_H