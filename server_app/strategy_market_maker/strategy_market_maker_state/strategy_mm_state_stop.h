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
    virtual Json get_info() override;

protected:
    virtual void handle_price_update(PriceUpdate& price_update) override;
    virtual void handle_trade_update(TradeUpdate& trade_update) override;
    virtual void handle_order_book_snapshot(OrderBookSnapShot* snapshot) override;
    virtual void handle_order_update(Order& order) override;
};
