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