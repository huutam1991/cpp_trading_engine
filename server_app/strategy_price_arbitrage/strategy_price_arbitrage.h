#pragma once

#include <app_constants.h>
#include <strategy/strategy_base.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

inline constexpr char PriceArbitrageName[] = STRATEGY_PRICE_ARBITRAGE_NAME;

class StrategyPriceArbitrage : public StrategyBase<StrategyPriceArbitrageConfig, PriceArbitrageName, EventBaseID::PRICE_ARBITRAGE_STRATEGY>
{
protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void on_config_change(StrategyPriceArbitrageConfig new_config) override;

private:
    Json get_orders_chain();
};
