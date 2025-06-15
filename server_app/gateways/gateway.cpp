#include <gateways/gateway.h>
#include <coroutine/event_base_manager.h>

Gateway::Gateway() : m_event_base {
    EventBaseManager::get_event_base_by_id(EventBaseID::GATEWAY) // Default is GATEWAY
}
{}

void Gateway::init()
{
    m_exchange_id = get_exchange();
    m_gateway_name = get_name();

    // Create instrument list by symbol and by exchange symbol
    auto& instruments_by_symbol = Instrument::get_instrument_list_by_exchange(m_exchange_id, Instrument::StoreType::BY_SYMBOL);
    load_instruments(instruments_by_symbol);
    Instrument::create_instrument_list_by_exchange_symbol(m_exchange_id);
    
    // Print instruments by symbol
    for (auto& [symbol, ins] : instruments_by_symbol)
    {
        // spdlog::debug("INSTRUMENT load - by symbol: {}, instrument: {}", symbol, ins.to_json());
    }

    // Print instruments by exchange symbol
    auto& instruments_by_exchange_symbol = Instrument::get_instrument_list_by_exchange(m_exchange_id, Instrument::StoreType::BY_EXCHANGE_SYMBOL);
    for (auto& [symbol, ins] : instruments_by_exchange_symbol)
    {
        // spdlog::debug("INSTRUMENT load - by exchange symbol: {}, instrument: {}", symbol, ins.to_json());
    }
}

void Gateway::load_instruments(std::unordered_map<std::string, Instrument>& instruments)
{
    m_instruments = SavableObject<Instrument>::load_objects_map<std::string>(INSTRUMENT_DB_NAME, m_gateway_name, "symbol");
    
    for (auto& [symbol, savable_instrument] : m_instruments)
    {
        instruments.insert(std::make_pair(symbol, savable_instrument.object));
    }
}

void Gateway::register_price_update(std::function<void(std::string,double)> price_update_callback)
{
    m_price_update_callback = std::move(price_update_callback);
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
    place_on_exchange(std::move(order)).start_running_on(m_event_base);
}

void Gateway::cancel(Order order)
{
    cancel_on_exchange(std::move(order)).start_running_on(m_event_base);
}

Instrument* Gateway::get_instrument_by_symbol(const std::string& symbol)
{
    auto it = m_instruments.find(symbol);
    if (it != m_instruments.end())
    {
        return &it->second.object;
    }
    else
    {
        throw std::runtime_error("Cannot find instrument symbol: " + symbol);
    }

    return nullptr;
}