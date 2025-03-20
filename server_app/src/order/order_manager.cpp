#include <order/order_manager.h>
#include <measure_time.h>

void OrderManager::init()
{
    m_order_list = m_order_data_model_helper.load_order();
}

void OrderManager::register_order_update(std::function<void(Order&)> order_update_callback)
{
    m_order_update_callback = order_update_callback;
}

void OrderManager::update_order(Order order)
{
    TaskVoid task = handle_update_order(order);
    task.start_running_on(
        EventBaseManager::instance()
            .get_event_base_by_id(EventBaseID::ORDER)
    );
}

Future<Order> OrderManager::get_order_data(OrderId order_id)
{
    return Future<Order>([this, order_id](Future<Order>::FutureValue value)
    {
        if (m_order_list.find(order_id) != m_order_list.end())
        {
            value.set_value(m_order_list[order_id]);
        }
    });
}

OrderId OrderManager::generate_order_id()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

    return static_cast<OrderId>(nanos);
}

Order OrderManager::find_order_by_id(OrderId order_id)
{
    MeasureTime g("Get order by id", MeasureUnit::MICROSECOND);
    if (m_order_list.find(order_id) == m_order_list.end())
    {
        m_order_list.insert(std::make_pair(order_id, Order()));
    }

    return m_order_list[order_id];
}

TaskVoid OrderManager::handle_update_order(Order order)
{
    MeasureTime a("Handle order update OrderManager", MeasureUnit::MICROSECOND);
    Order current_order_data = find_order_by_id(order.order_id);

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

    // Inform about order to strategy
    if (order.status == Order::Status::NEW || order.status == Order::Status::CANCELED || order.status == Order::Status::FILLED)
    {
        m_order_update_callback(order);
    }

    // Save order to DB, using DataModel implemented in OrderDataModelHelper
    m_order_data_model_helper.update_order(order);

    co_return;
}
