#include <coroutine/event_base_manager.h>
#include <enum_reflect/enum_reflect.h>

#include <gateways/gateway.h>
#include <order/simulator_order.h>

Gateway::Gateway() : m_event_base {
    EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY) // Default is GATEWAY
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

void Gateway::add_account(std::shared_ptr<AccountBase> account)
{
    account->m_order_entry = get_order_entry(account);
    m_accounts[account->get_key_name()] = account;
}

void Gateway::remove_account(std::shared_ptr<AccountBase> account)
{
    if (m_accounts.find(account->get_key_name()) == m_accounts.end())
    {
        spdlog::warn("Gateway::remove_account - Account with key name: [{}] not found in gateway: [{}]", account->get_key_name(), get_name());
        return;
    }

    account->m_order_entry = nullptr;
    m_accounts.erase(account->get_key_name());
}

std::vector<Instrument> Gateway::fetch_instruments()
{
    return {};
}
