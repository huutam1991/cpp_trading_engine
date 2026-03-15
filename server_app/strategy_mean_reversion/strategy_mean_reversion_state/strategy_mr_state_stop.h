#pragma once

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <strategy_mean_reversion/strategy_mean_reversion_config.h>
#include <strategy_mean_reversion/spread_capture_config.h>

class StrategyMeanReversionStateStop : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyMeanReversionConfig& m_config;
    std::vector<SpreadCaptureConfig>& m_spread_captures;
    double m_current_price = 0;

public:
    StrategyMeanReversionStateStop(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config, std::vector<SpreadCaptureConfig>& spread_captures);

    virtual void begin();
    virtual void end();
    virtual Task<void> update(StrategyUpdateData data);
    virtual Json get_info() override;

private:
    void handle_order_book_snapshot(OrderBookSnapShot* snapshot);
};
