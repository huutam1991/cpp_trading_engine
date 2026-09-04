#include <strategy/strategy_manager.h>
#include <order/simulator_order.h>
#include <strategy_price_arbitrage/strategy_price_arbitrage.h>
#include <strategy_buy_spot/strategy_buy_spot.h>
#include <strategy_market_maker/strategy_market_maker.h>
#include <strategy_trend_follow/strategy_trend_follow.h>
#include <strategy_mean_reversion/strategy_mean_reversion.h>

void StrategyManager::init()
{
    add_strategy_list();
    subscribe_data_update();
}

void StrategyManager::add_strategy_list()
{
    // m_strategy_list.push_back(std::make_unique<StrategyPriceArbitrage>());
    // m_strategy_list.push_back(std::make_unique<StrategyMarketMaker>());
    m_strategy_list.push_back(std::make_unique<StrategyMeanReversion>());
    // m_strategy_list.push_back(std::make_unique<StrategyBuySpot>());
    // m_strategy_list.push_back(std::make_unique<StrategyTrendFollow>());

    for (auto& strategy : m_strategy_list)
    {
        strategy->init().start_running_on(strategy->event_base);
    }
}

void StrategyManager::subscribe_data_update()
{
    // Subscribe order update from OrderManager
    OrderManager::instance().register_order_update([this](Order order)
    {
        for (auto& strategy : m_strategy_list)
        {
            strategy->update(order).start_running_on(strategy->event_base);
        }
    });

    // Subscribe order book update from OrderBookManager
    OrderBookManager::instance().register_update([this](OrderBookSnapShotObject snapshot)
    {
        if (SimulatorOrder::get_active())
        {
            SimulatorOrder::price_update(PriceUpdate{
                snapshot->instrument,
                snapshot->get_mid_price()
            });
        }

        for (auto& strategy : m_strategy_list)
        {
            strategy->update(snapshot).start_running_on(strategy->event_base);
        }
    });
}

void StrategyManager::public_data(StrategyUpdateData& data)
{
    if (SimulatorOrder::get_active())
    {
        if (std::holds_alternative<PriceUpdate>(data) == true)
        {
            SimulatorOrder::price_update(std::get<PriceUpdate>(data));
        }
    }

    for (auto& strategy : m_strategy_list)
    {
        strategy->update(data).start_running_on(strategy->event_base);
    }
}

Json StrategyManager::get_strategy_list()
{
    Json strategy_list;
    for (const auto& strategy : m_strategy_list)
    {
        strategy_list.push_back(strategy->get_name());
    }

    return strategy_list;
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
        {"message", "No strategy is running has name: [" + strategy_name + "]"}
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
        {"message", "No strategy is running has name: [" + strategy_name + "]"}
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
        {"message", "No strategy is running has name: [" + strategy_name + "]"}
    };
}