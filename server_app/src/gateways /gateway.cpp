#include <gateways/gateway.h>

Gateway::Gateway() : m_event_base {
    EventBaseManager::instance()
        .get_event_base_by_id(EventBaseID::STRATEGY) // Default is STRATEGY
    }
{}

void Gateway::register_price_update(std::function<void(double)> price_update_callback)
{
    m_price_update_callback = price_update_callback;
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