#pragma once

#include <unordered_map>
#include <array>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_mean_reversion/strategy_mean_reversion_config.h>

class StrategyMeanReversionStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyMeanReversionConfig& m_config;

    const Instrument* m_instrument = nullptr;
    double m_current_price = 0.0;

public:
    StrategyMeanReversionStateRun(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config);

    virtual void begin();
    virtual void end();
    virtual Json get_info() override;

private:
    Order m_buy_order = nullptr;
    Order m_sell_order = nullptr;

    Order get_limit_order(Order::Side side, double price, double quantity);

    void handle_price_update(PriceUpdate& price);
    void handle_trade_update(TradeUpdate& trade);
    void handle_order_book_snapshot(OrderBookSnapShot* snapshot);
    void handle_order_update(Order& order);
};
