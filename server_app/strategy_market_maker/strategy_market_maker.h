#pragma once

#include <app_constants.h>
#include <strategy/strategy_base.h>
#include <strategy_market_maker/strategy_market_maker_config.h>

inline constexpr char MarketMakerName[] = STRATEGY_MARKET_MAKER_NAME;

class StrategyMarketMaker : public StrategyBase<StrategyMarketMakerConfig, MarketMakerName, EventBaseID::MARKET_MAKER_STRATEGY>
{
protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void on_config_change(StrategyMarketMakerConfig new_config) override;
    
public:
    virtual Json get_info(Json& params) override;

private:
    Json get_orders_chain();
};
