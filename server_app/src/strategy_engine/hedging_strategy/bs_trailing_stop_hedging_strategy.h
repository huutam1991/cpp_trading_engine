#ifndef BS_TRAILING_STOP_HEDGING_STRATEGY_H
#define BS_TRAILING_STOP_HEDGING_STRATEGY_H

#include <strategy_engine/hedging_strategy/hedging_strategy.h>

class Json;
class OrderBook;

using namespace std;

class BSTrailingStopHedgingStrategy : public strategy_engine::HedgingStrategy
{
public:
    BSTrailingStopHedgingStrategy(const Json inputs);
    ~BSTrailingStopHedgingStrategy();

    void set_filled_price(const long double price);

    bool is_hedging_finished() override;
    bool is_hedging_started() override;
    void force_stop() override;

protected:
    // init strategy, init event handler.
    void on_init() override;

    // function is called during deinitialization and is the deinit event handler. 
    void on_deinit() override;

    // event is generated when a new tick for a symbol is received
    void on_tick() override;
    
private:
    void replace_new_order(const long double current_price);
    void update_new_price(const long double current_price);
    bool check_finish_hedging(Json& response);

    long double m_stop_price = 0.0;
    long double m_replace_price = 0.0;
    string m_side;
    long double m_tick_size = 0.0;
    long m_trailing_step_tick = 0;
    long m_trailing_stop_tick = 0;
    long m_trailing_limit_tick = 0;

    string m_symbol;
    shared_ptr<OrderBook> m_order_book_buffer;
    Json m_order_book;

    size_t m_order_manager_callback_id = -1;
    string m_client_order_id;
    long m_price_precision;
};

#endif