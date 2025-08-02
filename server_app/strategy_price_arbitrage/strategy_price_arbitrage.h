#pragma once

#include <app_constants.h>
#include <gateways/gateway_manager.h>
#include <strategy/strategy_base.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyPriceArbitrage : public StrategyBase<StrategyPriceArbitrageConfig, EventBaseID::PRICE_ARBITRAGE_STRATEGY>
{
protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void start() override;
    virtual void on_config_change(StrategyPriceArbitrageConfig new_config) override;

public:
    virtual Json get_info(Json& params) override;

private:
    std::shared_ptr<Gateway> m_gateway;
    Json get_orders_chain();
};
