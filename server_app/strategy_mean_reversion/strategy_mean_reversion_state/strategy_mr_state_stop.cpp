#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_stop.h>

StrategyMeanReversionStateStop::StrategyMeanReversionStateStop(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config)
    : m_gateway{gateway}, m_config{config}
{
}

void StrategyMeanReversionStateStop::begin()
{
    spdlog::info("StrategyMeanReversionStateStop - begin");
}

void StrategyMeanReversionStateStop::end()
{
    spdlog::info("StrategyMeanReversionStateStop - end");
}

Json StrategyMeanReversionStateStop::get_info()
{
    return {
        {"info", "TBD"}
    };
}

Task<void> StrategyMeanReversionStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
    }

    spdlog::debug("StrategyMeanReversionStateStop: do nothing, symbol: {}, price: {}", price_update.instrument->symbol, price_update.price);

    co_return;
}