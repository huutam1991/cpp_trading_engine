#pragma once

#include <data_model/savable_object.h>

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <order/order_manager.h>
#include <volume_stat/volume_stat.h>
#include <pnl/pnl.h>

class StrategyMarketMakerStateStop : public StrategyStateBase
{
    VolumeStat& m_volume_stat;
    PnL& m_pnl;

public:
    StrategyMarketMakerStateStop(VolumeStat& volume_stat, PnL& pnl);

    virtual void begin() override;
    virtual void end() override;
    virtual Task<void> update(StrategyUpdateData data) override;
    virtual Json get_info() override;

    // virtual Json get_open_orders() override;
};
