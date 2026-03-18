#pragma once

#include <data_model/savable_object.h>

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <order/order_manager.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyTrendFollowStateStop : public StrategyStateBase
{
public:
    StrategyTrendFollowStateStop();

    virtual void begin() override;
    virtual void end() override;
    virtual Json get_info() override;

protected:
    virtual void handle_price_update(PriceUpdate& price_update) override;
    virtual void handle_trade_update(TradeUpdate& trade_update) override;
    virtual void handle_order_book_snapshot(OrderBookSnapShot* snapshot) override;
    virtual void handle_order_update(Order& order) override;
};
