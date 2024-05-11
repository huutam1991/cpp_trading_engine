#ifndef BF_HEDGING_STRATEGY_H
#define BF_HEDGING_STRATEGY_H

#include <strategy_engine/hedging_strategy/hedging_strategy.h>

class Json;

using namespace std;

class BFHedgingStrategy : public strategy_engine::HedgingStrategy
{
public:
    BFHedgingStrategy(const Json inputs);
    ~BFHedgingStrategy();

    bool is_hedging_finished() { return m_is_hedging_finished;}
    bool is_hedging_started() { return true;}
    void force_stop() {}

protected:
    // init strategy, init event handler.
    void on_init() override;

    // function is called during deinitialization and is the deinit event handler. 
    void on_deinit() override;

    // event is generated when a new tick for a symbol is received
    void on_tick() override;
        
private:
    void open_hedging_order();

    string m_side;
    string m_symbol;

    Json m_symbol_info;
};

#endif