#pragma once

#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state.h>
#include <unordered_map>
#include <array>

#include <order/order_manager.h>

class StrategyMeanReversionStateRun : public StrategyMeanReversionState
{
public:
    StrategyMeanReversionStateRun(std::shared_ptr<Gateway>& gateway, StrategyMeanReversionConfig& config);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyMeanReversionData data);

    virtual JsonNew get_open_orders() override;

private:
    double m_current_price = 0.0;
    bool is_taking_profit = false;

    struct OrderInfo
    {
        Order order;
        bool is_handeling = false;
    };

    // Current open orders by price
    std::unordered_map<double, OrderInfo> m_current_open_orders;

    // Generate order
    Order get_limit_buy_spot_order_by_price(double price);
    Order get_limit_sell_spot_order_by_price_and_quantity(double price, double quantity);

    void remove_open_order_by_price(double price);
    void check_place_order_at_price(double price);
    void check_cancel_order_at_price(double price);
    TaskVoid handle_price_update(MRPriceUpdate price);
    TaskVoid handle_order_update(Order& order);
};
