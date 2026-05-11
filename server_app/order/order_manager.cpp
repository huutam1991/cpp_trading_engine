#include <time/measure_time.h>
#include <coroutine/event_base_manager.h>

#include <order/order_manager.h>
#include <app_constants.h>

void OrderManager::init()
{
    m_order_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::ORDER);

    // Load all orders in DB to [m_order_list]
    auto orders_in_db = SavableObject<Order>::load_objects_map<OrderId>(ORDER_DB_NAME, "order_list", "order_id");
    for (auto& [order_id, order] : orders_in_db)
    {
        m_order_list.insert(std::make_pair(order_id, order));
    }
}

OrderId OrderManager::generate_order_id()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

    return static_cast<OrderId>(nanos);
}

std::vector<Order> OrderManager::get_all_orders()
{
    std::vector<Order> res;

    for (auto& [_, order] : m_order_list)
    {
        res.push_back(order);
    }

    return res;
}

std::vector<OrderId> OrderManager::get_open_orders()
{
    std::vector<OrderId> res;

    for (auto& [order_id, order] : m_order_list)
    {
        if (order.status == Order::Status::NEW)
        {
            res.push_back(order_id);
        }
    }

    return res;
}

void OrderManager::register_order_update(std::function<void(Order&)> order_update_callback)
{
    m_order_update_callback = std::move(order_update_callback);
}

void OrderManager::set_cancel_order(OrderId order_id)
{
    if (is_valid_order(order_id) == true)
    {
        auto it = m_order_list.find(order_id);
        Order order = it->second;

        // Set order's status to CANCELED
        // Then invoke method update_order(), this order will be removed and notify to the whole engine
        order.status = Order::Status::CANCELED;
        update_order(order);
    }
}

void OrderManager::update_order(Order order)
{
    Task<void> task = handle_update_order(order);
    task.start_running_on(m_order_event_base);
}

Order& OrderManager::get_order_by_id(OrderId order_id)
{
    // MeasureTime g("OrderManager - get_order_by_id", MeasureUnit::MICROSECOND);
    if (is_valid_order(order_id) == false)
    {
        Order new_order;
        new_order.order_id = order_id;
        m_order_list.insert(std::make_pair(order_id, new_order));
    }

    auto it = m_order_list.find(order_id);
    return it->second;
}

Task<void> OrderManager::update_order_in_db(Order order)
{
    static std::unordered_map<OrderId, SavableObject<Order>> m_order_db_cache;

    // Create SavableObject for order if not exist
    if (m_order_db_cache.find(order.order_id) == m_order_db_cache.end())
    {
        m_order_db_cache.insert(std::make_pair(order.order_id, SavableObject<Order>(ORDER_DB_NAME, "order_list")));
    }

    SavableObject<Order>& order_db = m_order_db_cache.at(order.order_id);
    order_db = order;

    // Check to remove oder if needed
    // If order is canceled or rejected, remove it from [m_order_db_cache]
    if (order_db->status == Order::Status::CANCELED)
    {
        // order_db.remove();
        m_order_db_cache.erase(order_db->order_id);
    }
    // For FILLED order, we can also remove it from [m_order_db_cache] to save space, but dont remove from DB
    else if (order_db->status == Order::Status::FILLED)
    {
        m_order_db_cache.erase(order_db->order_id);
    }

    co_return;
}

Task<void> OrderManager::handle_update_order(Order order)
{
    MeasureTime a("OrderManager - Handle order update", MeasureUnit::MICROSECOND);
    Order& current_order_data = get_order_by_id(order.order_id);

    if (order.status == Order::Status::FILLED || order.status == Order::Status::PARTIALLY_FILLED)
    {
        if (order.side == Order::Side::BUY)
        {
            // Buy order's [output_quantity] is from [filled_quantity]
            order.output_quantity = order.filled_quantity - order.fee;
            // Buy order's [volumn_in_quote_currency] is from [filled_quantity] * [filled_price] (without minus [fee])
            order.volumn_in_quote_currency = order.filled_quantity * order.filled_price;
        }
        else
        {
            // Sell order's [output_quantity] is from [filled_quantity] * [filled_price]
            order.output_quantity = order.filled_quantity * order.filled_price - order.fee;
            // Sell order's [output_quantity] is from [output_quantity]
            order.volumn_in_quote_currency = order.output_quantity;
        }

        // Update order's output data
        order.filled_quantity += current_order_data.filled_quantity;
        order.fee += current_order_data.fee;
        order.output_quantity += current_order_data.output_quantity;
        order.volumn_in_quote_currency += current_order_data.volumn_in_quote_currency;

        // If [filled_quantity] == [quantity], order's status is FILLED
        if (order.filled_quantity == order.quantity)
        {
            order.status = Order::Status::FILLED;
        }
    }

    // Save current order data to [m_order_list]
    current_order_data = order;

    // Inform about order to strategy
    if (order.status == Order::Status::NEW ||
        order.status == Order::Status::CANCELED ||
        order.status == Order::Status::REJECTED ||
        order.status == Order::Status::FILLED)
    {
        // Invoke callback
        if (m_order_update_callback != nullptr)
        {
            m_order_update_callback(order);
        }
    }

    // Finally, update order in DB
    update_order_in_db(order).start_running_on(m_order_event_base);

    co_return;
}
