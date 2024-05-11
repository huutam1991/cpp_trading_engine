#include <strategy_engine/base_strategy.h>
#include <data_feed/data_processor/data_storage/order_book_manager.h>

#include <utils.h>
#include <app_utils.h>
#include <timer.h>

using namespace std;
using namespace strategy_engine;

BaseStrategy::BaseStrategy(const Json inputs) :
    m_strategy_inputs (inputs)
{
    m_schedule_id = 0;
    
    // create exchange connector if need
    
}

BaseStrategy::~BaseStrategy()
{
    ADD_LOG("~BaseStrategy()");
    kill_event_timer();
}

void BaseStrategy::start()
{
    on_init();
    subscribe_symbols();
}

void BaseStrategy::stop()
{
    // multithread safe
    unique_lock lock(m_base_strategy_on_tick_mutex);

    unsubscribe_symbols();
    on_deinit();
}

void BaseStrategy::on_notify(void* data) 
{
    AppUtils::instance().get_app_pool()->execute_function([this]()
    {
        // multithread safe
        if (m_base_strategy_on_tick_mutex.try_lock())
        {
            on_tick();
            m_base_strategy_on_tick_mutex.unlock();
        }
    });
}

void BaseStrategy::subscribe_symbols()
{
    for(string symbol : m_symbol_list)
    {
        ADD_LOG("subscribe_symbols() " << symbol);
        OrderBookManager::instance().subscribe_symbol(symbol, this);
    }
}

void BaseStrategy::unsubscribe_symbols()
{
    for(string symbol : m_symbol_list)
    {
        ADD_LOG("unsubscribe_symbols() " << symbol);
        OrderBookManager::instance().unsubscribe_symbol(symbol, this);
    }
}

void BaseStrategy::set_event_timer(int miliseconds)
{
    kill_event_timer();
    m_schedule_id = Timer::instance().add_schedule_task([&]()
    {
        on_timer();
    }, miliseconds);
}

void BaseStrategy::kill_event_timer()
{
    if (m_schedule_id) 
    {
        Timer::instance().delete_schedule_task(m_schedule_id);
    }
}

void BaseStrategy::send_data_to_client_through_channel(const string& channel_name, 
    Json& data, const string& user_name)
{
    WebSocketServerType::instance().send_data_through_channel(
        channel_name,
        user_name,
        data
    );
}

Market BaseStrategy::get_market_id_from_name(const string& market_name)
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


