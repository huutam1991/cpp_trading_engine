#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_mean_reversion/strategy_mean_reversion_config.h>
#include <pnl/pnl.h>

class StrategyMeanReversionStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyMeanReversionConfig& m_config;
    SpreadCaptureConfigManager& m_spread_captures;

    const Instrument* m_instrument = nullptr;
    double m_current_price = 0.0;
    PnL m_pnl;

public:
    StrategyMeanReversionStateRun(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config, SpreadCaptureConfigManager& spread_captures);

    virtual void begin();
    virtual void end();
    virtual Json get_info() override;

private:
    Order m_buy_order = nullptr;
    Order m_sell_order = nullptr;

    Order get_limit_order(Order::Side side, double price, double quantity);

protected:
    virtual void handle_price_update(PriceUpdate& price_update) override;
    virtual void handle_trade_update(TradeUpdate& trade_update) override;
    virtual void handle_order_book_snapshot(OrderBookSnapShot* snapshot) override;
    virtual void handle_order_update(Order& order) override;
};
