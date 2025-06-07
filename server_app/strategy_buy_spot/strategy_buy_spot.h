#pragma once

#include <app_constants.h>
#include <strategy/strategy_base.h>
#include <strategy_buy_spot/strategy_buy_spot_config.h>

inline constexpr char BuySpotName[] = STRATEGY_BUY_SPOT_NAME;

class StrategyBuySpot : public StrategyBase<StrategyBuySpotConfig, BuySpotName, EventBaseID::BUY_SPOT_STRATEGY>
{
protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void on_config_change(StrategyBuySpotConfig new_config) override;
    
public:
    virtual Json get_info(Json& params) override;

private:
};
