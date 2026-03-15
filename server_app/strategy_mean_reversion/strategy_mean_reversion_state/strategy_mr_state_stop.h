#pragma once

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <strategy_mean_reversion/strategy_mean_reversion_config.h>

class StrategyMeanReversionStateStop : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyMeanReversionConfig& m_config;

public:
    StrategyMeanReversionStateStop(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config);

    virtual void begin();
    virtual void end();
    virtual Task<void> update(StrategyUpdateData data);
    virtual Json get_info() override;
};
