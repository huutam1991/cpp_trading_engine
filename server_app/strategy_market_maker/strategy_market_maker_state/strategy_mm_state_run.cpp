#include "strategy_mm_state_run.h"
#include <time/measure_time.h>
#include <utils/utils.h>

StrategyMarketMakerStateRun::StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, const StrategyMarketMakerConfig& config)
    : m_gateway{gateway}, m_config{config}
{
}

void StrategyMarketMakerStateRun::begin()
{
    on_config_change();
    spdlog::info("StrategyMarketMakerStateRun - begin");
}

void StrategyMarketMakerStateRun::end()
{
    spdlog::info("StrategyMarketMakerStateRun - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_instrument->exchange_symbol);
}

void StrategyMarketMakerStateRun::on_config_change()
{
    m_current_price = 0.0;
    m_instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
}

Order StrategyMarketMakerStateRun::get_buy_limit_order(double price, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        price,
        quantity
    );
}

Order StrategyMarketMakerStateRun::get_sell_limit_order(double price, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::SELL,
        Order::OrderType::LIMIT,
        price,
        quantity
    );
}

Order StrategyMarketMakerStateRun::get_market_buy_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity)
{
    double round_up_quantity = m_instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::BUY,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

Order StrategyMarketMakerStateRun::get_market_sell_spot_order_by_symbol_and_quantity(const std::string& symbol, double quantity)
{
    double round_up_quantity = m_instrument->get_round_up_quantity(quantity);

    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::SELL,
        Order::OrderType::MARKET,
        0.0, // since type is MARKET, no need to specify price
        round_up_quantity
    );
}

void StrategyMarketMakerStateRun::handle_price_update(PriceUpdate price_update)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_price_update");

    m_current_price = price_update.price;
}

void StrategyMarketMakerStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMarketMakerStateRun - handle_order_book_snapshot", MeasureUnit::MICROSECOND);

    double best_bid = snapshot->get_best_bid();
    double best_ask = snapshot->get_best_ask();

    spdlog::debug("StrategyMarketMakerStateRun - best_bid: {}, best_ask: {}", best_bid, best_ask);
}

void StrategyMarketMakerStateRun::handle_order_update(Order& order)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_order_update");

    // NEW - do nothing
    if (order.status == Order::Status::NEW)
    {
    }
    // FILLED - update order
    else if (order.status == Order::Status::FILLED)
    {
        // 1st order (LIMIT)
        if (order.side == Order::Side::BUY)
        {
        }
        else if (order.side == Order::Side::SELL)
        {
        }
    }
}

Task<void> StrategyMarketMakerStateRun::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        handle_price_update(price_update);
    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        handle_order_book_snapshot(snapshot);

        OrderBookSnapShotPool::release(snapshot);
    }
    else
    {
        Order order = std::get<Order>(data);
        handle_order_update(order);
    }

    co_return;
}