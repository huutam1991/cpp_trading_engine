#pragma once

#include <app_constants.h>
#include <strategy/strategy_base.h>
#include <strategy_market_maker/strategy_market_maker_config.h>

class StrategyMarketMaker : public StrategyBase<StrategyMarketMakerConfig, EventBaseID::MARKET_MAKER_STRATEGY>
{
protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void on_config_change(StrategyMarketMakerConfig new_config) override;

public:
    virtual JsonNew get_info(JsonNew& params) override;

private:
    JsonNew get_orders_chain();
};
