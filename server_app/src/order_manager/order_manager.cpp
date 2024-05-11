#include <mongo_db/mongo_db.h>
#include <order_manager/order_manager.h>

size_t OrderManager::subscribe_user_data(std::function<void(Json&)> callback)
{
    std::unique_lock lock(m_order_manager_mutex);

    m_callback_id++;
    m_subscribed_callback_list.insert(std::make_pair(m_callback_id, callback));

    ADD_LOG("Order Callback size = " << m_subscribed_callback_list.size());

    return m_callback_id;
}

/*void OrderManager::set_callback_by_order_id(const std::string& symbol, size_t order_id, std::function<void(Json&)> callback)
{
    std::unique_lock lock(m_order_manager_mutex);

    std::string key = symbol + "_" + std::to_string(order_id);
    m_callback_list_by_order_id.insert(std::make_pair(key, callback));
}*/

void OrderManager::unsubscribe_user_data(size_t callback_id)
{
    if (m_subscribed_callback_list.find(callback_id) != m_subscribed_callback_list.end())
    {
        std::unique_lock lock(m_order_manager_mutex);
        m_subscribed_callback_list.erase(callback_id);
        ADD_LOG("Order Callback size = " << m_subscribed_callback_list.size());
    }
}

/*void OrderManager::remove_callback_by_order_id(const std::string& symbol, size_t order_id)
{
    std::string key = symbol + "_" + std::to_string(order_id);
    if (m_callback_list_by_order_id.find(key) != m_callback_list_by_order_id.end())
    {
        std::unique_lock lock(m_order_manager_mutex);
        m_callback_list_by_order_id.erase(key);
    }
}*/

void OrderManager::invoke_callback(Json& order)
{
    // std::unique_lock lock(m_order_manager_mutex);

    for (auto it = m_subscribed_callback_list.begin(); it != m_subscribed_callback_list.end(); it++)
    {
        it->second(order);
    }
}

/*void OrderManager::format_order(Json& order)
{
    order["quantity"] = order["origQty"];
    order.remove_field("selfTradePreventionMode");
    order.remove_field("workingTime");
    order.remove_field("clientOrderId");
    order.remove_field("cummulativeQuoteQty");
    order.remove_field("orderListId");
    order.remove_field("origQty");
    order.remove_field("executedQty");
    order.remove_field("timeInForce");
    order.remove_field("fills");
}*/

/*void OrderManager::copy_order(Json& order, Json& copy_order)
{
    copy_order.for_each_with_key([&order](const std::string& key, Json& val)
    {
        order[key] = val;
    });
}*/

std::string OrderManager::get_string_order_id(Json& order)
{
    long order_id = order["orderId"];
    return std::to_string(order_id);
}

std::unique_ptr<OrderByMarket>& OrderManager::get_order_by_user(const std::string& user_id, const std::string& db_name)
{
    if (m_order_by_user_list.find(user_id) == m_order_by_user_list.end())
    {
        m_order_by_user_list.insert(std::make_pair(user_id, std::unordered_map<std::string, std::unique_ptr<OrderByMarket>>()));
    }
    std::unordered_map<std::string, std::unique_ptr<OrderByMarket>>& list_by_db_name = m_order_by_user_list[user_id];

    if (list_by_db_name.find(db_name) == list_by_db_name.end())
    {
        list_by_db_name.insert(std::make_pair(db_name, std::make_unique<OrderByMarket>(db_name)));
    }

    return list_by_db_name[db_name];
}

Json OrderManager::get_order_list(const std::string& user_id, const std::string& db_name, long from, long to)
{
    std::unique_lock lock(m_order_manager_mutex);
    return get_order_by_user(user_id, db_name)->get_order_list(user_id, db_name, from, to);
}

Json OrderManager::get_order(const std::string& user_id, const std::string& db_name, long order_id)
{
    Json order = get_order_by_user(user_id, db_name)->get_order_by_id(order_id);

    if (order.is_null() == true)
    {
        order = MongoDB::instance()
            .set_db_and_collection(db_name, "order")
            .find_one("orderId", order_id);
    }

    return order;
}

void OrderManager::update_order(const std::string& db_name, Json& order)
{
    // {
    //     std::unique_lock lock(m_order_manager_mutex);

    //     // Format order
    //     //format_order(order);

    //     std::string user_id = order["user_id"];
    //     get_order_by_user(user_id, db_name)->update_order(order);
    // }
    // lock.unlock();

    // Invoke order update callback
    invoke_callback(order);
}

void OrderManager::clean_all_cached_orders()
{
    m_order_by_user_list.clear();
}