#include <coroutine/event_base_manager.h>
#include <enum_reflect/enum_reflect.h>

#include <gateways/order_entry.h>
#include <order/simulator_order.h>

void OrderEntry::check_remove_canceled_orders(std::string symbol)
{
    // Get open orders from gateway
    auto task = get_open_orders_on_exchange(std::move(symbol));
    auto future = task.get_future();

    task.start_running_on(m_event_base);
    std::unordered_set<OrderId> open_orders_from_gateway = future.get();

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

void OrderEntry::cancel_all(std::string symbol)
{
    if (SimulatorOrder::get_active())
    {
        SimulatorOrder::cancel_all(std::move(symbol));
        return;
    }

   cancel_all_on_exchange(std::move(symbol)).start_running_on(m_event_base);
}

void OrderEntry::place(Order order)
{
    if (SimulatorOrder::get_active())
    {
        SimulatorOrder::place(std::move(order));
        return;
    }

    place_on_exchange(std::move(order)).start_running_on(m_event_base);
}

void OrderEntry::cancel(Order order)
{
    if (SimulatorOrder::get_active())
    {
        SimulatorOrder::cancel(std::move(order));
        return;
    }

    cancel_on_exchange(std::move(order)).start_running_on(m_event_base);
}