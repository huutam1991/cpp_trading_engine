#pragma once

#include <mutex>
#include <queue>

#include <utils/util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>
#include <coroutine/task.h>
#include <coroutine/future.h>

#include <gateways/gateway.h>
#include <strategy/strategy_base.h>
#include <strategy_mean_reversion/strategy_mean_reversion_config.h>
#include <strategy_mean_reversion/spread_capture_config.h>

class StrategyMeanReversion : public StrategyBase<StrategyMeanReversionConfig, EventBaseID::MEAN_REVERSION_STRATEGY>
{
    // Gateway
    std::shared_ptr<Gateway> m_gateway;
    SpreadCaptureConfigManager m_spread_captures;

protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() override;
    virtual void on_config_change(StrategyMeanReversionConfig new_config) override;
    virtual Json get_info(Json& params) override;

    void run();
    void stop();

};