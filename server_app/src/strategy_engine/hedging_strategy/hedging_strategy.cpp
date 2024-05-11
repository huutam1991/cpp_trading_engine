#include <strategy_engine/hedging_strategy/hedging_strategy.h>
#include <json/json.h>

using namespace strategy_engine;

HedgingStrategy::HedgingStrategy(const Json inputs) :
    BaseStrategy(inputs)
{
    m_current_placed_order_id = 0;
    m_is_hedging_finished = false;
    m_is_hedging_started = false;

    m_strategy_id = (long)m_strategy_inputs["id"];
}

HedgingStrategy::~HedgingStrategy()
{
}