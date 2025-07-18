#include <strategy/strategy_manager.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage.h>
#include <strategy_market_maker/strategy_market_maker.h>
#include <strategy_buy_spot/strategy_buy_spot.h>

void StrategyManager::init()
{
    add_strategy_list();
    subscribe_data_update();
}

void StrategyManager::add_strategy_list()
{
    // m_strategy_list.push_back(std::make_unique<StrategyPriceArbitrage>());
    // m_strategy_list.push_back(std::make_unique<StrategyMarketMaker>());
    m_strategy_list.push_back(std::make_unique<StrategyBuySpot>());

    for (auto& strategy : m_strategy_list)
    {
        strategy->init().start_running_on(strategy->event_base);
    }
}

void StrategyManager::subscribe_data_update()
{
    // Add price callback + subscribe to symbol
    auto gateway = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);
    gateway->register_price_update([this](const Instrument* instrument, double price)
    {
        for (auto& strategy : m_strategy_list)
        {
            strategy->update(PriceUpdate{instrument, price}).start_running_on(strategy->event_base);
        }
    });

    // Subscribe order update from OrderManager
    OrderManager::instance().register_order_update([this](Order& order)
    {
        for (auto& strategy : m_strategy_list)
        {
            strategy->update(order).start_running_on(strategy->event_base);
        }
    });
}

Json StrategyManager::get_config_by_strategy(const std::string& strategy_name)
{
    for (auto& strategy : m_strategy_list)
    {
        if (strategy->get_name() == strategy_name)
        {
            return strategy->get_config();
        }
    }

    // If cannot find [strategy_name], return error
    return {
        {"code", -1},
        {"message", "no strategy with name: [" + strategy_name + "]"}
    };
}

Json StrategyManager::update_config_by_strategy(const std::string& strategy_name, Json& data)
{
    for (auto& strategy : m_strategy_list)
    {
        if (strategy->get_name() == strategy_name)
        {
            strategy->update_config(data);
            
            return {
                {"code", 0},
            };
        }
    }

    // If cannot find [strategy_name], return error
    return {
        {"code", -1},
        {"message", "no strategy with name: [" + strategy_name + "]"}
    };
}

Json StrategyManager::get_info(const std::string& strategy_name, Json& params)
{
    for (auto& strategy : m_strategy_list)
    {
        if (strategy->get_name() == strategy_name)
        {
            return strategy->get_info(params);
        }
    }

    // If cannot find [strategy_name], return error
    return {
        {"code", -1},
        {"message", "no strategy with name: [" + strategy_name + "]"}
    };
}