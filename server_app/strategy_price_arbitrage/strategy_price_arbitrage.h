#pragma once

#include <strategy/strategy_base.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

inline constexpr char PriceArbitrageName[] = "PriceArbitrage";

using StrategyPriceArbitrage = StrategyBase<StrategyPriceArbitrageConfig, PriceArbitrageName, EventBaseID::PRICE_ARBITRAGE_STRATEGY>;
