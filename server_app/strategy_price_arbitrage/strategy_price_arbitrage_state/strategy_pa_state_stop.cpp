#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state_stop.h>

StrategyPriceArbitrageStateStop::StrategyPriceArbitrageStateStop(std::shared_ptr<Gateway> gateway, StrategyPriceArbitrageConfig& config)
    : m_gateway{gateway}, m_config{config}
{
}

void StrategyPriceArbitrageStateStop::begin()
{
    ADD_LOG("StrategyPriceArbitrageStateStop - begin");

    // Re-subscribe symbols
    auto ins1 = m_gateway->get_instrument_by_symbol(m_config.symbol_1);
    auto ins2 = m_gateway->get_instrument_by_symbol(m_config.symbol_2);
    m_gateway->subscribe_symbol({ins1->exchange_id, ins2->exchange_id});
}

void StrategyPriceArbitrageStateStop::end()
{
    ADD_LOG("StrategyPriceArbitrageStateStop - end");
}

TaskVoid StrategyPriceArbitrageStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
    }

    spdlog::info("StrategyPriceArbitrageStateStop - run: Do nothing, symbol: {}, price: {} ", price_update.symbol, price_update.price);

    co_return;
}

// Json StrategyPriceArbitrageStateStop::get_open_orders()
// {
//     return Json::create_array();
// }