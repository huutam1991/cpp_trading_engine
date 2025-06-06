#pragma once

#include <app_constants.h>
#include <strategy/strategy_base.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

inline constexpr char PriceArbitrageName[] = STRATEGY_PRICE_ARBITRAGE_NAME;

using StrategyPriceArbitrage = StrategyBase<StrategyPriceArbitrageConfig, PriceArbitrageName, EventBaseID::PRICE_ARBITRAGE_STRATEGY>;
