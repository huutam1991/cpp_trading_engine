#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_run.h>
#include <time/measure_time.h>

StrategyMeanReversionStateRun::StrategyMeanReversionStateRun(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config)
    : m_gateway{gateway}, m_config{config}
{
}

void StrategyMeanReversionStateRun::begin()
{
    m_current_price = 0.0;
    spdlog::info("StrategyMeanReversionStateRun - begin");
}

void StrategyMeanReversionStateRun::end()
{
    spdlog::info("StrategyMeanReversionStateRun - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_config.symbol);
    m_current_open_orders.clear();
    is_taking_profit = false;
}

Json StrategyMeanReversionStateRun::get_info()
{
    return {
        {"info", "TBD"}
    };
}

Task<void> StrategyMeanReversionStateRun::handle_price_update(PriceUpdate price_update)
{

    co_return;
}

Task<void> StrategyMeanReversionStateRun::handle_order_update(Order& order)
{

    co_return;
}

Task<void> StrategyMeanReversionStateRun::update(StrategyUpdateData data)
{

    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        handle_price_update(price_update);
    }
    else if (std::holds_alternative<TradeUpdate>(data))
    {
        TradeUpdate trade = std::get<TradeUpdate>(data);
        std::string side = trade.is_buy ? "BUY" : "SELL";

        // spdlog::info("StrategyMarketMakerStateRun: trade update, symbol: {}, side: {}, price: {}, quantity: {}",
        //     trade.instrument->symbol, side, trade.price, trade.quantity);

    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        // handle_order_book_snapshot(snapshot);

        OrderBookSnapShotPool::release(snapshot);
    }
    else
    {
        Order order = std::get<Order>(data);
        handle_order_update(order);
    }


    co_return;
}
