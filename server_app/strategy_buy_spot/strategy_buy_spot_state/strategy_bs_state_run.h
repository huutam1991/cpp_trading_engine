#pragma once

#include <unordered_map>

#include <data_model/savable_object.h>

#include <order/order_manager.h>
#include <gateways/gateway.h>
#include <strategy/strategy_state_base.h>
#include <strategy_buy_spot/strategy_buy_spot_config.h>
#include <strategy_buy_spot/strategy_buy_buy_point.h>

class StrategyBuySpotStateRun : public StrategyStateBase
{
    std::shared_ptr<Gateway> m_gateway;
    const StrategyBuySpotConfig& m_config;

public:
    StrategyBuySpotStateRun(std::shared_ptr<Gateway> gateway, const StrategyBuySpotConfig& config);

    virtual void begin() override;
    virtual void end() override;
    virtual TaskVoid update(StrategyUpdateData data) override;

    // virtual Json get_open_orders() override;

private:
    Instrument* m_instrument = nullptr;
    double m_current_price = 0.0;
    double m_lower_nearest_price = 0.0;
    std::string m_strategy_buy_spot_db_name = STRATEGY_BUY_SPOT_NAME + std::string("_strategy");

    // List of buy points
    std::unordered_map<double, SavableObject<BuyPoint>> m_buy_points;

    void on_config_change();

    // Generate buy point
    void add_buy_point_at_price(double price);
    SavableObject<BuyPoint>* get_buy_point_by_price(double price);

    // Generate order
    Order get_limit_buy_spot_order_by_price(double price);
    Order get_cancel_order(OrderId order_id);

    double get_a_price_point();
    double get_lower_nearest_price();
    void add_new_buy_points();
    void update_buy_orders();

    TaskVoid handle_price_update(PriceUpdate price);
    TaskVoid handle_order_update(Order& order);
};
