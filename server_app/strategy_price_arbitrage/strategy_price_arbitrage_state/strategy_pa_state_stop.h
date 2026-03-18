#pragma once

#include <strategy/strategy_state_base.h>
#include <gateways/gateway.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage_config.h>

class StrategyPriceArbitrageStateStop : public StrategyStateBase
{
public:
    StrategyPriceArbitrageStateStop();

    virtual void begin() override;
    virtual void end() override;
    virtual Json get_info() override { return {}; }

protected:
    virtual void handle_price_update(PriceUpdate& price_update) override;
    virtual void handle_trade_update(TradeUpdate& trade_update) override;
    virtual void handle_order_book_snapshot(OrderBookSnapShot* snapshot) override;
    virtual void handle_order_update(Order& order) override;

    // virtual Json get_open_orders() override;
};
