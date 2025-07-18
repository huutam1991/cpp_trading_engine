

#include <strategy_buy_spot/strategy_buy_spot.h>
#include <mongo_db/mongo_db.h>

// StrategyState
#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_stop.h>
#include <strategy_buy_spot/strategy_buy_spot_state/strategy_bs_state_run.h>

std::unordered_map<StrategyState, StrategyStateBase*> StrategyBuySpot::init_states()
{
    std::unordered_map<StrategyState, StrategyStateBase*> strategy_states;

    // For now, only use Binance
    m_gateway = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);

    strategy_states[StrategyState::STOP] = new StrategyBuySpotStateStop();
    strategy_states[StrategyState::RUN] = new StrategyBuySpotStateRun(m_gateway, get_config_reference());

    return strategy_states;
}

void StrategyBuySpot::start()
{
    // Subscribe symbols
    auto instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.object.instrument_type, m_config.object.symbol);
    m_gateway->subscribe_instruments({instrument});
}

void StrategyBuySpot::on_config_change(StrategyBuySpotConfig new_config)
{
    spdlog::debug("Update config for StrategyBuySpot");

    if (new_config.is_running == true)
    {
        m_current_state = StrategyStateData{StrategyState::RUN};
    }
    else
    {
        m_current_state = StrategyStateData{StrategyState::STOP};
    }

    // Re-subscribe symbols
    auto instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.object.instrument_type, m_config.object.symbol);
    m_gateway->subscribe_instruments({instrument});
}

Json StrategyBuySpot::get_info(Json& params)
{
    if ((std::string)params["type"] == "orders_chain")
    {
        // return get_orders_chain();
    }
    if ((std::string)params["type"] == "profit")
    {
        return get_profit();
    }

    return {};
}

Json StrategyBuySpot::get_profit()
{
    double total_profit = 0.0;
    MongoDB::instance().set_db_and_collection(STRATEGY_BUY_SPOT_NAME + std::string("_strategy"), "buy_points").find_many().for_each
    (
        [&total_profit](Json& buy_point)
        {
            double profit = buy_point["profit"];
            total_profit += profit;
        }
    );

    return {
        {"profit", total_profit}
    };
}