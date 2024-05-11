#ifndef BASE_STRATEGY_H
#define BASE_STRATEGY_H

#include <json/json.h>
#include <observer.h>
#include <mutex>
#include <app_constants.h>

enum StrategyState
{
    STATE_NONE			    = 0,
    STATE_SCANNING			= 1,
    STATE_TRADING 		    = 2,
    STATE_HEDGING 		    = 3,
    STATE_DELETED 		    = 4
};

using namespace std;

namespace strategy_engine
{
    class BaseStrategy : public observer::Observer
    {
    public:
        BaseStrategy(const Json inputs);
        ~BaseStrategy();

        // start the strategy
        void start();

        // stop the strategy
        void stop();

        // got notification from observer
        void on_notify(void* data) override;

    protected:
        // init strategy, init event handler.
        virtual void on_init() {};

        // function is called during deinitialization and is the deinit event handler. 
        virtual void on_deinit() {};

        // event is generated when a new tick for a symbol is received
        virtual void on_tick() {};

        // function is called when the Timer event occurs (miliseconds)
        virtual void on_timer() {};

        virtual void add_symbols_to_order_book() {};
        virtual void remove_symbols_from_order_book() {};

        void subscribe_symbols();
        void unsubscribe_symbols();

        void set_event_timer(int miliseconds);
        void kill_event_timer();
        
        Market get_market_id_from_name(const string& market_name);

        void send_data_to_client_through_channel(const string& channel_name, 
                Json& data, const string& user_name = "root");
   
    protected:
        mutex m_base_strategy_on_tick_mutex;
        mutex m_strategy_order_mutex;
    
        Json m_strategy_inputs;    
        size_t m_schedule_id;
        string m_exchange_name;
        vector<string> m_symbol_list;
    };
}

#endif