#include <order/order_manager.h>
#include <measure_time.h>

void OrderManager::init()
{
    m_order_list = m_order_data_model_helper.load_order();
    m_order_event_base = EventBaseManager::instance().get_event_base_by_id(EventBaseID::ORDER);
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
        if (order.status == Order::Status::NEW)
        {
            res.push_back(order_id);
        }
    }

    return res;
}

void OrderManager::register_order_update(std::function<void(Order&)> order_update_callback)
{
    m_order_update_callback = order_update_callback;
}

void OrderManager::set_cancel_order(OrderId order_id)
{
    if (is_valid_order(order_id) == true)
    {
        Order order = m_order_list[order_id];

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

void OrderManager::add_order_future_value(Future<Order>::FutureValue value, OrderId order_id, Order::Status status)
{
    TaskVoid task = handle_add_order_future_value(value, order_id, status);
    task.start_running_on(m_order_event_base);
}

void OrderManager::check_set_future_value_for_order(OrderId order_id, Order::Status status)
{
    MeasureTime t("check_set_future_value_for_order");
    Order& order = get_order_by_id(order_id);

    // If order's status is not the expected one, do nothing
    if (order.status != status)
    {
        return;
    }

    auto key = std::make_pair(order_id, status);

    // If there's no order future value in [m_order_future_value], do nothing
    if (m_order_future_value.find(key) == m_order_future_value.end())
    {
        return;
    }

    // Otherwise, update value for order's future value
    std::vector<Future<Order>::FutureValue>& order_future_value_list = m_order_future_value[key];
    for (Future<Order>::FutureValue& order_future_value : order_future_value_list)
    {
        order_future_value.set_value(order);
    }

    // Remove all of current order's future value, as they are all set
    order_future_value_list.resize(0);
}

Future<Order> OrderManager::wait_for_order_status(OrderId order_id, Order::Status status)
{
    // If order's status is the expected one, return it
    if (is_valid_order(order_id) == true)
    {
        Order& order = m_order_list[order_id];
        if (order.status == status)
        {
            return Future<Order>(order);
        }
    }

    // Otherwise return in future
    return Future<Order>([this, order_id, status](Future<Order>::FutureValue value)
    {
        add_order_future_value(value, order_id, status);
    });
}

Order& OrderManager::get_order_by_id(OrderId order_id)
{
    MeasureTime g("Get order by id", MeasureUnit::MICROSECOND);
    if (is_valid_order(order_id) == false)
    {
        m_order_list.insert(std::make_pair(order_id, Order()));
    }

    return m_order_list[order_id];
}

TaskVoid OrderManager::handle_add_order_future_value(Future<Order>::FutureValue value, OrderId order_id, Order::Status status)
{
    MeasureTime a("handle_add_order_future_value", MeasureUnit::NANOSECOND);
    auto key = std::make_pair(order_id, status);
    if (m_order_future_value.find(key) == m_order_future_value.end())
    {
        m_order_future_value.insert(std::make_pair(key, std::vector<Future<Order>::FutureValue>{}));
    }

    m_order_future_value[key].push_back(value);

    // Always return order status REJECTED
    key = std::make_pair(order_id, Order::Status::REJECTED);
    if (m_order_future_value.find(key) == m_order_future_value.end())
    {
        m_order_future_value.insert(std::make_pair(key, std::vector<Future<Order>::FutureValue>{}));
    }

    m_order_future_value[key].push_back(value);

    // Check to set future value for order
    check_set_future_value_for_order(order_id, status);

    co_return;
}

TaskVoid OrderManager::handle_update_order(Order order)
{
    MeasureTime a("Handle order update OrderManager", MeasureUnit::MICROSECOND);
    Order& current_order_data = get_order_by_id(order.order_id);

    if (order.status == Order::Status::FILLED || order.status == Order::Status::PARTIALLY_FILLED)
    {
        // Buy order's [output_quantity] is from [filled_quantity]
        if (order.side == Order::Side::BUY)
        {
            order.output_quantity = order.filled_quantity - order.commission_amount;
        }
        // Sell order's [output_quantity] is from [filled_quantity] * [filled_price]
        else
        {
            order.output_quantity = order.filled_quantity * order.filled_price - order.commission_amount;
        }

        // Update order's output data
        order.filled_quantity += current_order_data.filled_quantity;
        order.commission_amount += current_order_data.commission_amount;
        order.output_quantity += current_order_data.output_quantity;

        // If [filled_quantity] == [quantity], order's status is FILLED
        if (order.filled_quantity == order.quantity)
        {
            order.status = Order::Status::FILLED;
        }
    }

    // Update order
    current_order_data = order;

    // If order is canceled remove it from [m_order_list]
    if (order.status == Order::Status::CANCELED)
    {
        m_order_list.erase(order.order_id);
    }

    // Inform about order to strategy
    if (order.status == Order::Status::NEW ||
        order.status == Order::Status::CANCELED ||
        order.status == Order::Status::REJECTED ||
        order.status == Order::Status::FILLED)
    {
        // Invoke callback
        m_order_update_callback(order);

        // Check to set future value for order
        check_set_future_value_for_order(order.order_id, order.status);
    }

    // Save order to DB, using DataModel implemented in OrderDataModelHelper
    m_order_data_model_helper.update_order(order);

    co_return;
}
