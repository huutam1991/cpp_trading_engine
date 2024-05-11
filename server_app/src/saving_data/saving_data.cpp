#include <saving_data/saving_data.h>
#include <data_feed/data_processor/data_storage/order_book_manager.h>

#include <utils.h>
#include <app_utils.h>
#include <timer.h>

using namespace std;

SavingData::SavingData(const Json inputs) :
    m_inputs (inputs)
{        
}

SavingData::~SavingData()
{
    ADD_LOG("~SavingData()");
}

void SavingData::start()
{
    on_init();
    subscribe_symbols();
}

void SavingData::stop(bool delete_data)
{
    // multithread safe
    unique_lock lock(m_on_tick_mutex);

    unsubscribe_symbols();
    on_deinit();
}

void SavingData::on_notify(void* data) 
{
    AppUtils::instance().get_app_pool()->execute_function([this]()
    {
        // multithread safe
        if (m_on_tick_mutex.try_lock())
        {
            on_tick();
            m_on_tick_mutex.unlock();
        }
    });
}

void SavingData::subscribe_symbols()
{
    for(string symbol : m_symbol_list)
    {
        ADD_LOG("subscribe_symbols() " << symbol);
        OrderBookManager::instance().subscribe_symbol(symbol, this);
    }
}

void SavingData::unsubscribe_symbols()
{
    for(string symbol : m_symbol_list)
    {
        ADD_LOG("unsubscribe_symbols() " << symbol);
        OrderBookManager::instance().unsubscribe_symbol(symbol, this);
    }
}

void SavingData::send_data_to_client_through_channel(const string& channel_name, 
    Json& data, const string& user_name)
{
    WebSocketServerType::instance().send_data_through_channel(
        channel_name,
        user_name,
        data
    );
}

Market SavingData::get_market_id_from_name(const string& market_name)
{
    Market market_id;
    if (market_name == BINANCE_SPOT_ABBREVIATION_NAME)
        market_id = BINANCE_SPOT;
    else if (market_name == BINANCE_FUTURES_ABBREVIATION_NAME)
        market_id = BINANCE_FUTURES;
    else if (market_name == BINANCE_NAV_ABBREVIATION_NAME)
        market_id = BINANCE_BLVT;

    return market_id;
}

Json SavingData::get_inputs()
{
    return m_inputs;
}

