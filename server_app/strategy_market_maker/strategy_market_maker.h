#pragma once

#include <app_constants.h>
#include <gateways/gateway_manager.h>
#include <volume_stat/volume_stat.h>
#include <pnl/pnl.h>
#include <strategy/strategy_base.h>
#include <strategy_market_maker/strategy_market_maker_config.h>

class StrategyMarketMaker : public StrategyBase<StrategyMarketMakerConfig, EventBaseID::MARKET_MAKER_STRATEGY>
{
    std::shared_ptr<Gateway> m_gateway;
    VolumeStat m_volume_stat;
    PnL m_pnl;

protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void on_config_change(StrategyMarketMakerConfig new_config) override;

public:
    virtual Json get_info(Json& params) override;

private:
};
