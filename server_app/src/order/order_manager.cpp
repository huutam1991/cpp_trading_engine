#include <order/order_manager.h>

void OrderManager::init()
{
    m_order_list = DataModel::load_data_model_map<OrderId>(ORDER_DB_NAME, "order_list", "order_id");

    for (const auto& [order_id, order] : m_order_list)
    {
        ADD_LOG("order_id: " << order_id << ", Order: " << order);
    }
}

OrderId OrderManager::generate_order_id()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

    return static_cast<OrderId>(nanos);
}

void OrderManager::create_order_data_model(OrderId order_id)
{
    DataModel order_dm(ORDER_DB_NAME, "order_list");
    Order order;
    order.order_id = order_id;

    // Init data for [order_dm] and insert too [m_order_list]
    order_dm = order.to_json();
    m_order_list.insert(std::make_pair(order_id, order_dm));
}

DataModel OrderManager::find_order_by_id(OrderId order_id)
{
    if (m_order_list.find(order_id) == m_order_list.end())
    {
        create_order_data_model(order_id);
    }

    return m_order_list[order_id];
}

void OrderManager::update_order(Order order)
{
    DataModel order_dm = find_order_by_id(order.order_id);

    order_dm = order.to_json();
}