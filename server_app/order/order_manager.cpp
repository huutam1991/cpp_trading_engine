#include <time/measure_time.h>
#include <coroutine/event_base_manager.h>

#include <order/order_manager.h>
#include <app_constants.h>

void OrderManager::init()
{
    m_order_list = SavableObject<Order>::load_objects_map<OrderId>(ORDER_DB_NAME, "order_list", "order_id");
    m_order_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::ORDER);

    // Print out order list
    for (auto& [order_id, order] : m_order_list)
    {
        ADD_LOG("Order: " << order.to_json());
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
    TaskVoid task = handle_update_order(order);
    task.start_running_on(m_order_event_base);
}

SavableObject<Order>& OrderManager::get_order_by_id(OrderId order_id)
{
    // MeasureTime g("Get order by id", MeasureUnit::MICROSECOND);
    if (is_valid_order(order_id) == false)
    {
        m_order_list.insert(std::make_pair(order_id, SavableObject<Order>(ORDER_DB_NAME, "order_list")));
    }

    auto it = m_order_list.find(order_id);
    return it->second;
}

TaskVoid OrderManager::handle_update_order(Order order)
{
    MeasureTime a("Handle order update OrderManager 1", MeasureUnit::MICROSECOND);
    SavableObject<Order>& current_order_data = get_order_by_id(order.order_id);

    if (order.status == Order::Status::FILLED || order.status == Order::Status::PARTIALLY_FILLED)
    {
        if (order.side == Order::Side::BUY)
        {
            // Buy order's [output_quantity] is from [filled_quantity]
            order.output_quantity = order.filled_quantity - order.commission_amount;
            // Buy order's [volumn_in_quote_currency] is from [filled_quantity] * [filled_price] (without minus [commission_amount])
            order.volumn_in_quote_currency = order.filled_quantity * order.filled_price;
        }
        else
        {
            // Sell order's [output_quantity] is from [filled_quantity] * [filled_price]
            order.output_quantity = order.filled_quantity * order.filled_price - order.commission_amount;
            // Sell order's [output_quantity] is from [output_quantity]
            order.volumn_in_quote_currency = order.output_quantity;
        }

        // Update order's output data
        order.filled_quantity += current_order_data.object.filled_quantity;
        order.commission_amount += current_order_data.object.commission_amount;
        order.output_quantity += current_order_data.object.output_quantity;
        order.volumn_in_quote_currency += current_order_data.object.volumn_in_quote_currency;

        // If [filled_quantity] == [quantity], order's status is FILLED
        if (order.filled_quantity == order.quantity)
        {
            order.status = Order::Status::FILLED;
        }
    }

    // Update order
    current_order_data = order;

    // Inform about order to strategy
    if (order.status == Order::Status::NEW ||
        order.status == Order::Status::CANCELED ||
        order.status == Order::Status::REJECTED ||
        order.status == Order::Status::FILLED)
    {
        // Invoke callback
        m_order_update_callback(order);
    }

    // If order is canceled or rejected, remove it from [m_order_list]
    if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        current_order_data.remove();
        m_order_list.erase(order.order_id);
    }

    co_return;
}
