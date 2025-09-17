#include <order/simulator_order.h>

SavableObject<SimulatorOrder::SimulatorConfig>& SimulatorOrder::get_config()
{
    static SavableObject<SimulatorConfig> m_config{SavableObject<SimulatorConfig>::load_single_object("simulator_order", "config")};
    return m_config;
}

EventBase* SimulatorOrder::get_event_base()
{
    static EventBase* m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::ORDER);
    return m_event_base;
}

std::unordered_map<const Instrument*, std::unordered_map<OrderId, Order>>& SimulatorOrder::get_order_list()
{
    static std::unordered_map<const Instrument*, std::unordered_map<OrderId, Order>> m_order_list;
    return m_order_list;
}

void SimulatorOrder::init()
{
    get_config();
    spdlog::info("SimulatorOrder initialized");
}

void SimulatorOrder::set_active(bool active)
{
    SimulatorConfig config = get_config().object;
    config.state = active ? SimulatorState::ACTIVE : SimulatorState::INACTIVE;

    SavableObject<SimulatorConfig>& new_config = get_config();
    new_config = config;
}

bool SimulatorOrder::get_active()
{
    return get_config().object.state == SimulatorState::ACTIVE;
}

Json SimulatorOrder::get_info()
{
    return get_config().to_json();
}

void SimulatorOrder::place(Order order)
{
    auto task = execute_place(std::move(order));
    task.start_running_on(get_event_base());
}

void SimulatorOrder::cancel(Order order)
{
    auto task = execute_cancel(std::move(order));
    task.start_running_on(get_event_base());
}

void SimulatorOrder::cancel_all(std::string symbol)
{
    auto task = execute_cancel_all(std::move(symbol));
    task.start_running_on(get_event_base());
}

void SimulatorOrder::price_update(PriceUpdate data)
{
    auto task = execute_price_update(data);
    task.start_running_on(get_event_base());
}

Task<void> SimulatorOrder::execute_place(Order order)
{
    order.status = Order::Status::NEW;
    auto order_list = get_order_list();
    order_list[order.instrument].insert(std::make_pair(order.order_id, order));

    OrderManager::instance().update_order(order);

    co_return;
}

Task<void> SimulatorOrder::execute_cancel(Order order)
{
    order.status = Order::Status::CANCELED;
    auto order_list = get_order_list();
    order_list[order.instrument].erase(order.order_id);
    OrderManager::instance().update_order(order);

    co_return;
}

Task<void> SimulatorOrder::execute_cancel_all(std::string symbol)
{
    // TBD
    co_return;
}

Task<void> SimulatorOrder::execute_price_update(PriceUpdate data)
{
    auto order_list = get_order_list();
    if (order_list.find(data.instrument) == order_list.end())
    {
        co_return;
    }

    std::vector<OrderId> filled_orders;
    auto current_orders = order_list[data.instrument];
    for (auto& [order_id, order] : current_orders)
    {
        if (order.type == Order::LIMIT)
        {
            if ((order.side == Order::BUY && data.price <= order.price) ||
                (order.side == Order::SELL && data.price >= order.price))
            {
                filled_orders.push_back(order_id);
            }
        }
        else if (order.type == Order::MARKET)
        {
            filled_orders.push_back(order_id);
        }
    }

    for (OrderId order_id : filled_orders)
    {
        Order order = current_orders[order_id];
        order_list[data.instrument].erase(order_id);

        order.status = Order::FILLED;
        order.filled_price = data.price;
        order.filled_quantity = order.quantity;
        order.fee = 0.0; // No fee in simulator

        OrderManager::instance().update_order(order);
    }

    co_return;
}