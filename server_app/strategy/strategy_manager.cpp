#include <strategy/strategy_manager.h>

void StrategyManager::init()
{
    // Add price callback + subscribe to symbol
    auto gateway = GatewayManager::instance().get_gateway(GatewayEnum::BINANCE);
    gateway->register_price_update([this](std::string symbol, double price)
    {
        for (auto& strategy : m_strategy_list)
        {
            strategy->update(PriceUpdate{std::move(symbol), price}).start_running_on(strategy->event_base);
        }
    });
    gateway->subscribe_symbol({"BTCUSDT"});

    // Subscribe order update from OrderManager
    OrderManager::instance().register_order_update([this](Order& order)
    {
        for (auto& strategy : m_strategy_list)
        {
            strategy->update(order).start_running_on(strategy->event_base);
        }
    });
}