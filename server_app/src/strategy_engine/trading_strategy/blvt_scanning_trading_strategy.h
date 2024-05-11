#ifndef BLVT_SCANNING_TRADING_STRATEGY_H
#define BLVT_SCANNING_TRADING_STRATEGY_H

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

class BLVTScanningTradingStrategy : public strategy_engine::TradingStrategy
{
public:
    BLVTScanningTradingStrategy(const Json inputs);
    ~BLVTScanningTradingStrategy();

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

    void order_filled_callback(long double price);

private:
    void read_strategy_inputs();

    void open_order_and_hedge(const string& type, Json& data);
    void open_order(Json& query_json);
    Json create_spot_hedging_inputs(Json& order);
    Json create_future_hedging_inputs(Json& order);
    
    bool hit_expected_profit(const string& type, Json& data);
    
    long double calculate_theorical_bid(const long double currency_bid_price);

    long double calculate_theorical_ask(const long double currency_ask_price);

    Json calculate_trading_data();

    string m_currency_symbol;
    Json m_currency_symbol_info;
    Json m_currency_order_book;
    shared_ptr<OrderBook> m_currency_order_book_buffer;
    
    long double m_initial_multiply_ratio;
    long double m_initial_offset;
};

#endif