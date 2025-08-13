#pragma once

#include <app_constants.h>
#include <gateways/gateway_manager.h>
#include <strategy/strategy_base.h>
#include <strategy_trend_follow/strategy_trend_follow_config.h>

class StrategyTrendFollow : public StrategyBase<StrategyTrendFollowConfig, EventBaseID::TREND_FOLLOW_STRATEGY>
{
    std::shared_ptr<Gateway> m_gateway;

protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void on_config_change(StrategyTrendFollowConfig new_config) override;

public:
    virtual Json get_info(Json& params) override;

private:
};
