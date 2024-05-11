#ifndef SAVING_DATA_H
#define SAVING_DATA_H

#include <json/json.h>
#include <observer.h>
#include <mutex>
#include <app_constants.h>

enum SavingState
{
    STATE_NONE			    = 0,
    STATE_SAVING			= 1,
    STATE_DELETED 		    = 2
};

using namespace std;

class SavingData : public observer::Observer
{
public:
    SavingData(const Json inputs);
    ~SavingData();

    // start the strategy
    void start();

    // stop the strategy
    void stop(bool delete_data = false);

    // got notification from observer
    void on_notify(void* data) override;

    Json get_inputs();

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
    
    Market get_market_id_from_name(const string& market_name);

    void send_data_to_client_through_channel(const string& channel_name, 
            Json& data, const string& user_name = "root");

protected:
    mutex m_on_tick_mutex;

    Json m_inputs;    
    string m_exchange_name;
    vector<string> m_symbol_list;
};

#endif