#ifndef STRATEGY_ENGINE_HEDGING_STRATEGY_H
#define STRATEGY_ENGINE_HEDGING_STRATEGY_H

#include <strategy_engine/base_strategy.h>
#include <app_constants.h>

class Json;

namespace strategy_engine
{
    class HedgingStrategy : public BaseStrategy
    {
    public:
        HedgingStrategy(const Json inputs);
        ~HedgingStrategy();

        // check finish hedging
        virtual bool is_hedging_finished() = 0;

        // check start hedging
        virtual bool is_hedging_started() = 0;

        // check start hedging
        virtual void force_stop() = 0;

    protected:
        long m_strategy_id;

        bool m_is_hedging_finished;
        bool m_is_hedging_started;
        long m_current_placed_order_id;

        Market m_market_id;

    };
}

#endif