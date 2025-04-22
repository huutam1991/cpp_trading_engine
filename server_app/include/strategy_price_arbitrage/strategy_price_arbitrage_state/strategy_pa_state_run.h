#ifndef STRATEGY_PA_STATE_RUN_H
#define STRATEGY_PA_STATE_RUN_H

#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state.h>
#include <unordered_map>
#include <array>

#include <order/order_manager.h>

class StrategyPriceArbitrageStateRun : public StrategyPriceArbitrageState
{
public:
    StrategyPriceArbitrageStateRun(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyPriceArbitrageData data);

private:
    double m_symbol_2_price;
    double m_current_price = 0.0;

    // Current open orders by price
    std::unordered_map<double, Order> m_current_open_orders;

    // Generate order
    Order get_limit_buy_spot_order_by_price(double price);
    Order get_market_buy_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity);
    Order get_market_sell_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity);

    void remove_open_order_by_price(double price);
    void check_place_order_at_price(double price);
    void check_cancel_order_at_price(double price);
    void update_orders_at_price(double price);
    TaskVoid handle_price_update(PriceUpdate price);
    TaskVoid handle_order_update(Order& order);
};

#endif //STRATEGY_PA_STATE_RUN_H