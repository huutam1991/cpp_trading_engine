#pragma once

#include <app_constants.h>
#include <strategy/strategy_base.h>
#include <strategy_buy_spot/strategy_buy_spot_config.h>
#include <gateways/gateway_manager.h>

class StrategyBuySpot : public StrategyBase<StrategyBuySpotConfig, EventBaseID::BUY_SPOT_STRATEGY>
{
private:
    std::shared_ptr<Gateway> m_gateway;

    JsonNew get_profit();

protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void start() override;
    virtual void on_config_change(StrategyBuySpotConfig new_config) override;

public:
    virtual JsonNew get_info(JsonNew& params) override;
};
