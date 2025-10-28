#include <coroutine/event_base_manager.h>
#include <enum_reflect/enum_reflect.h>

#include <gateways/gateway.h>
#include <order/simulator_order.h>

Gateway::Gateway() : m_event_base {
    EventBaseManager<EventBaseID>::get_event_base_by_id(EventBaseID::GATEWAY) // Default is GATEWAY
}
{}

std::string Gateway::get_name()
{
    return std::string(enum_reflect::enum_name(m_exchange_id));
}

void Gateway::init()
{
    m_exchange_id = get_exchange();

    // Load cache instruments
    Instrument::CacheInstruments& cache_instruments = Instrument::load_cache_instruments(m_exchange_id);

    // Fetch new instruments if cache is empty
    if (cache_instruments.empty())
    {
        spdlog::info("Gateway::init - Fetching instruments for exchange: {}", get_name());
        std::vector<Instrument> instruments = fetch_instruments();

        for (const Instrument& instrument : instruments)
        {
            spdlog::info("Gateway::init - Adding instrument: {}", instrument.symbol.to_string());
            Instrument::add_instrument_to_cache(m_exchange_id, instrument);
        }
    }
    else
    {
        spdlog::info("Gateway::init - Using cached instruments for exchange: {}", get_name());
    }
}

std::vector<Instrument> Gateway::fetch_instruments()
{
    return {};
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
    if (SimulatorOrder::get_active())
    {
        SimulatorOrder::cancel_all(std::move(symbol));
        return;
    }

   cancel_all_on_exchange(std::move(symbol)).start_running_on(m_event_base);
}

void Gateway::place(Order order)
{
    if (SimulatorOrder::get_active())
    {
        SimulatorOrder::place(std::move(order));
        return;
    }

    place_on_exchange(std::move(order)).start_running_on(m_event_base);
}

void Gateway::cancel(Order order)
{
    if (SimulatorOrder::get_active())
    {
        SimulatorOrder::cancel(std::move(order));
        return;
    }

    cancel_on_exchange(std::move(order)).start_running_on(m_event_base);
}