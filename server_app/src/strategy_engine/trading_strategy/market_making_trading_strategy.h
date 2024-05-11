#ifndef MARKET_MAKING_TRADING_STRATEGY_H
#define MARKET_MAKING_TRADING_STRATEGY_H

#include <strategy_engine/trading_strategy/trading_strategy.h>
#include <strategy_engine/hedging_strategy/hedging_strategy.h>

// #include <data_model/data_model.h>
// #include <app_constants.h>
// #include <constants.h>

class Json;
class OrderBook;
// class DataModel;

using namespace std;
// using namespace strategy_engine;

class MarketMakingTradingStrategy : public strategy_engine::TradingStrategy
{
public:
    MarketMakingTradingStrategy(const Json inputs);
    ~MarketMakingTradingStrategy();

    ResponseStatusCode update_strategy_inputs(Json& strategy_inputs) override;
    void stop_trading(bool force_stop_hedging = false) override;
        
protected:
    // init strategy, init event handler.
    void on_init() override;

    // function is called during deinitialization and is the deinit event handler. 
    void on_deinit() override;

    // event is generated when a new tick for a symbol is received
    void on_tick() override;

    void add_symbols_to_order_book() override;
    void remove_symbols_from_order_book() override;

private:
    void read_strategy_inputs();
        
    Json calculate_trading_data();
    long double calculate_theorical_bid(const long double currency_bid_price);
    long double calculate_theorical_ask(const long double currency_ask_price);

    bool can_execute_market_making(Json& calculated_data);
    bool open_market_making_order(Json& calculated_data);
    void process_execution_report(Json& trade);
    void open_hedging_order(long double quantity);

    void reset_trade_params();
    void finish_trading_order();

    mutex m_trade_mutex;
    
    string m_currency_symbol;
    Json m_currency_symbol_info;
    Json m_currency_order_book;
    shared_ptr<OrderBook> m_currency_order_book_buffer;
                
    long double m_initial_multiply_ratio;
    long double m_initial_offset;
    long m_theo_price_tick;
    long double m_minimum_main_order_size;
    long double m_minimum_currency_order_size;

    long double m_main_qty;
    long double m_main_qty_left;
    long double m_main_qty_error;
    long double m_currency_qty;
    long double m_cumulative_main_qty;
    long double m_last_cumulative_currency_qty;
    long double m_last_price;
    long double m_theo_price;

    set<string> m_orders_placed_set;
};

#endif