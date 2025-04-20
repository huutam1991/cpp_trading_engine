#ifndef STRATEGY_PA_STATE_RUN_H
#define STRATEGY_PA_STATE_RUN_H

#include <strategy_price_arbitrage/strategy_price_arbitrage_state/strategy_pa_state.h>
#include <unordered_map>

#include <order/order_manager.h>

class StrategyPriceArbitrageStateRun : public StrategyPriceArbitrageState
{
public:
    StrategyPriceArbitrageStateRun(std::shared_ptr<Gateway>& gateway, StrategyPriceArbitrageConfig& config);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(StrategyData data);

    TaskVoid handle_price_update(double price);
    TaskVoid handle_order_update(Order& order);

private:
    Order m_current_order;
    std::unordered_map<OrderId, Order> m_current_open_orders;

    void remove_open_order_id(OrderId order_id);
    Order get_limit_buy_spot_order_by_price(double price);
    Order get_market_buy_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity);
    Order get_market_sell_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity);
};

#endif //STRATEGY_PA_STATE_RUN_H