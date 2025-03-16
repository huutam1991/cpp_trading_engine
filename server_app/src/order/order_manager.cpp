#include <order/order_manager.h>
#include <measure_time.h>

void OrderManager::init()
{
    m_order_list = DataModel::load_data_model_map<OrderId>(ORDER_DB_NAME, "order_list", "order_id");

    for (const auto& [order_id, order] : m_order_list)
    {
        ADD_LOG("order_id: " << order_id << ", Order: " << order);
    }

    // Start running on EventBaseID::ORDER
    auto task = check_update_order();
    task.start_running_on(EventBaseManager::instance().get_event_base_by_id(EventBaseID::ORDER));
}

void OrderManager::update_order(Order order)
{
    std::unique_lock lock(m_order_manager_mutex);
    m_order_update_queue.push(order);

    // Inform that there's new order in [m_order_update_queue]
    m_has_order_update.set_value(true);
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

void OrderManager::handle_update_order(Order order)
{
    DataModel order_dm = find_order_by_id(order.order_id);

    // If order is canceled, remove it
    if (order.status == Order::Status::CANCELED)
    {
        m_order_list.erase(order.order_id);
        order_dm.remove();

        return;
    }

    order_dm = order.to_json();
}

Future<bool> OrderManager::wait_new_order_update()
{
    return Future<bool>([this](Future<bool>::FutureValue value)
    {
        m_has_order_update = value;
    });
}

TaskVoid OrderManager::check_update_order()
{
    while (true)
    {
        co_await wait_new_order_update();

        while (m_order_update_queue.size() > 0)
        {
            // Get order from [m_order_update_queue]
            Order order = m_order_update_queue.front();

            // Update this order
            handle_update_order(order);

            // Remove order from [m_order_update_queue]
            m_order_update_queue.pop();
        }
    }
}