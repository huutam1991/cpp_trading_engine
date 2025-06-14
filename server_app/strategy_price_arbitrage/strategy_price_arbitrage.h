#pragma once

#include <app_constants.h>
#include <gateways/gateway_manager.h>
#include <strategy/strategy_base.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

inline constexpr char PriceArbitrageName[] = STRATEGY_PRICE_ARBITRAGE_NAME;

class StrategyPriceArbitrage : public StrategyBase<StrategyPriceArbitrageConfig, PriceArbitrageName, EventBaseID::PRICE_ARBITRAGE_STRATEGY>
{
protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void on_config_change(StrategyPriceArbitrageConfig new_config) override;
    
public:
    virtual Json get_info(Json& params) override;

private:
    Json get_orders_chain();
};
