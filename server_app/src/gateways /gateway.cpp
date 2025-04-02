#include <gateways/gateway.h>

Gateway::Gateway() : m_event_base {
    EventBaseManager::instance()
        .get_event_base_by_id(EventBaseID::GATEWAY) // Default is GATEWAY
    }
{}

void Gateway::register_price_update(std::function<void(double)> price_update_callback)
{
    m_price_update_callback = price_update_callback;
}

void Gateway::check_remove_canceled_orders(std::string symbol)
{
    // Get open orders from gateway
    std::unordered_set<OrderId> open_orders_from_gateway = get_open_orders_on_exchange(std::move(symbol))
        .start_running_on(m_event_base)
        .get();

    // Get open orders from OrderManager
    std::vector<OrderId> open_orders = OrderManager::instance().get_open_orders();

    // Set cancel for order that doesn't exist on [open_orders_from_gateway] (because they are canceled somehow)
    for (OrderId order_id : open_orders)
    {
        if (open_orders_from_gateway.find(order_id) == open_orders_from_gateway.end())
        {
            OrderManager::instance().set_cancel_order(order_id);
        }
    }
}

void Gateway::cancel_all(std::string symbol)
{
   cancel_all_on_exchange(std::move(symbol)).start_running_on(m_event_base);
}

void Gateway::place_none_wait(Order order)
{
    place_on_exchange(order).start_running_on(m_event_base);
}

Task<Order> Gateway::place(Order order, Order::Status wait_status)
{
    place_on_exchange(order).start_running_on(m_event_base);

    co_return co_await OrderManager::instance().wait_for_order_status(order.order_id, wait_status);
}