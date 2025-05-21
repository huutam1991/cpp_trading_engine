#include <order/order_data_model_helper.h>

#include <measure_time.h>

std::unordered_map<OrderId, Order> OrderDataModelHelper::load_order()
{
    std::unordered_map<OrderId, Order> res;

    m_order_list = DataModel::load_data_model_map<OrderId>(ORDER_DB_NAME, "order_list", "order_id");
    for (auto& [order_id, order] : m_order_list)
    {
        ADD_LOG("order_id: " << order_id << ", Order: " << order);
        res.insert(std::make_pair(order_id, Order::from_json(order.get_data())));
    }

    return res;
}

void OrderDataModelHelper::create_order_data_model(OrderId order_id)
{
    DataModel order_dm(ORDER_DB_NAME, "order_list");
    Order order;
    order.order_id = order_id;

    // Init data for [order_dm] and insert too [m_order_list]
    order_dm = order.to_json();
    m_order_list.insert(std::make_pair(order_id, order_dm));
}

DataModel OrderDataModelHelper::get_order_by_id(OrderId order_id)
{
    if (m_order_list.find(order_id) == m_order_list.end())
    {
        create_order_data_model(order_id);
    }

    return m_order_list[order_id];
}

void OrderDataModelHelper::update_order(Order& order)
{
    TaskVoid task = task_update_order(order);
    task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::ORDER_DATA_MODEL_HELPER)
    );
}

TaskVoid OrderDataModelHelper::task_update_order(Order order)
{
    DataModel order_dm = get_order_by_id(order.order_id);

    // // Remove order if it's status is CANCELED
    // if (order.status == Order::Status::CANCELED)
    // {
    //     order_dm.remove();
    //     m_order_list.erase(order.order_id);

    //     co_return;
    // }

    // otherwise, save order
    order_dm = order.to_json();

    co_return;
}