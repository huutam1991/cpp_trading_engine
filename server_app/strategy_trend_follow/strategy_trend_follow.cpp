

#include <strategy_trend_follow/strategy_trend_follow.h>

// StrategyState
#include <strategy_trend_follow/strategy_trend_follow_state/strategy_trend_follow_state_run.h>
#include <strategy_trend_follow/strategy_trend_follow_state/strategy_trend_follow_state_stop.h>

std::unordered_map<StrategyState, StrategyStateBase*> StrategyTrendFollow::init_states()
{
    std::unordered_map<StrategyState, StrategyStateBase*> strategy_states;

    // For now, only use Binance
    m_gateway = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);
    const Instrument* instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.object.symbol);
    m_gateway->subscribe_instruments({instrument});

    strategy_states[StrategyState::RUN] = new StrategyTrendFollowStateRun(m_gateway, get_config_reference());
    strategy_states[StrategyState::STOP] = new StrategyTrendFollowStateStop();

    return strategy_states;
}

void StrategyTrendFollow::on_config_change(StrategyTrendFollowConfig new_config)
{
    spdlog::debug("Update config for StrategyTrendFollow");

    if (new_config.is_running == true)
    {
        m_current_state = StrategyStateData{StrategyState::RUN};

        // Update config for state Run
        auto state_run = (StrategyTrendFollowStateRun*)m_states[StrategyState::RUN];
        state_run->on_config_change();
    }
    else
    {
        m_current_state = StrategyStateData{StrategyState::STOP};
    }

    // Re-subscribe symbols
    auto instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.object.symbol);
    m_gateway->subscribe_instruments({instrument});
}

Json StrategyTrendFollow::get_info(Json& params)
{
    return m_states[m_current_state.object.state]->get_info();
}

