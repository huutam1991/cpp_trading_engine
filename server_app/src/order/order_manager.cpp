#include <order/order_manager.h>

void OrderManager::init()
{
    m_order_list = DataModel::get_data_model_map<OrderId>(ORDER_DB_NAME, "order_list", "order_id");
}

OrderId OrderManager::generate_order_id()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

    return static_cast<OrderId>(nanos);
}