#include <time/measure_time.h>
#include <coroutine/event_base_manager.h>

#include <order/order_manager.h>
#include <app_constants.h>

void OrderManager::init()
{
    m_order_list = SavableObject<Order>::load_objects_map<OrderId>(ORDER_DB_NAME, "order_list", "order_id");
    m_order_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::ORDER);

    std::vector<OrderId> filled_orders;

    // Print out order list
    for (auto& [order_id, order] : m_order_list)
    {
        spdlog::debug("Order: {}", order.to_json());
        if (order.object.status == Order::Status::FILLED)
        {
            filled_orders.push_back(order_id);
        }
    }

    // Remove FILLED orders from m_order_list to save space
    for (auto& order_id : filled_orders)
    {
        m_order_list.erase(order_id);
    }
}

OrderId OrderManager::generate_order_id()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

    return static_cast<OrderId>(nanos);
}

std::vector<OrderId> OrderManager::get_open_orders()
{
    std::vector<OrderId> res;

    for (auto& [order_id, order] : m_order_list)
    {
        if (order.object.status == Order::Status::NEW)
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

Order OrderManager::get_order_by_id(OrderId order_id)
{
    // MeasureTime g("Get order by id", MeasureUnit::MICROSECOND);
    if (is_valid_order(order_id) == false)
    {
        Order order = Order();
        order.order_id = order_id;

        return order;
    }

    auto it = m_order_list.find(order_id);
    return it->second.object;
}

Task<void> OrderManager::update_order_in_db(Order order)
{
    if (is_valid_order(order.order_id) == true)
    {
        auto it = m_order_list.find(order.order_id);
        it->second = order;
    }
    else
    {
        m_order_list.insert(std::make_pair(order.order_id, SavableObject<Order>(ORDER_DB_NAME, "order_list", order)));
    }

    co_return;
}

Task<void> OrderManager::check_to_remove_order(OrderId order_id)
{
    SavableObject<Order>& order = m_order_list.at(order_id);

    // If order is canceled or rejected, remove it from [m_order_list]
    if (order.object.status == Order::Status::CANCELED || order.object.status == Order::Status::REJECTED)
    {
        order.remove();
        m_order_list.erase(order_id);
    }
    // For FILLED order, we can also remove it from [m_order_list] to save space, but dont remove from DB
    else if (order.object.status == Order::Status::FILLED)
    {
        m_order_list.erase(order_id);
    }

    co_return;
}

Task<void> OrderManager::handle_update_order(Order order)
{
    MeasureTime a("OrderManager - Handle order update", MeasureUnit::MICROSECOND);
    Order current_order_data = get_order_by_id(order.order_id);

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

    // Update order in DB
    update_order_in_db(order).start_running_on(m_order_event_base);

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

    // Check to remove order if needed
    co_await check_to_remove_order(order.order_id);

    co_return;
}
