

#include <strategy_market_maker/strategy_market_maker.h>

// StrategyState
#include <strategy_market_maker/strategy_market_maker_state/strategy_mm_state_run.h>
#include <strategy_market_maker/strategy_market_maker_state/strategy_mm_state_stop.h>

std::unordered_map<StrategyState, StrategyStateBase*> StrategyMarketMaker::init_states()
{
    std::unordered_map<StrategyState, StrategyStateBase*> strategy_states;

    // For now, only use Binance
    m_gateway = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);
    const Instrument* instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.object.symbol);
    m_gateway->subscribe_instruments({instrument});

    strategy_states[StrategyState::RUN] = new StrategyMarketMakerStateRun(m_gateway, get_config_reference());
    strategy_states[StrategyState::STOP] = new StrategyMarketMakerStateStop();

    return strategy_states;
}

void StrategyMarketMaker::on_config_change(StrategyMarketMakerConfig new_config)
{
    spdlog::debug("Update config for StrategyMarketMaker");

    if (new_config.is_running == true)
    {
        m_current_state = StrategyStateData{StrategyState::RUN};

        // Update config for state Run
        auto state_run = (StrategyMarketMakerStateRun*)m_states[StrategyState::RUN];
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

Json StrategyMarketMaker::get_info(Json& params)
{
    auto state_run = (StrategyMarketMakerStateRun*)m_states[StrategyState::RUN];

    return {
        {"inventory", state_run->get_inventory()}
    };
}

