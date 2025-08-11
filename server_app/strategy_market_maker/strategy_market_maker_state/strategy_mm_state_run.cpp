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

void StrategyMarketMakerStateRun::quote_block_orders_at_price(double price)
{
    MeasureTime t("StrategyMarketMakerStateRun - quote_block");

    // Calculate the price for buy and sell orders
    double bid_price_gap = m_config.price_gap;
    double ask_price_gap = m_config.price_gap;
    double inventory_in_blocks = std::abs(m_inventory / m_config.orders_each_side_per_block);

    if (inventory_in_blocks >= m_config.inventory_skew_ratio)
    {
        if (m_inventory > 0)
        {
            // If inventory is positive, we need to sell more
            bid_price_gap *= inventory_in_blocks;
        }
        else
        {
            // If inventory is negative, we need to buy more
            ask_price_gap *= inventory_in_blocks;
        }
    }

    for (size_t i = 1; i <= m_config.orders_each_side_per_block; i++)
    {
        // Buy orders
        double buy_price = price - (i * bid_price_gap);
        Order buy_order = get_buy_limit_order(buy_price, m_config.volumn);
        m_gateway->place(buy_order);

        // Sell orders
        double sell_price = price + (i * ask_price_gap);
        Order sell_order = get_sell_limit_order(sell_price, m_config.volumn);
        m_gateway->place(sell_order);
    }
}

void StrategyMarketMakerStateRun::handle_price_update(PriceUpdate price_update)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_price_update");
}

void StrategyMarketMakerStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMarketMakerStateRun - handle_order_book_snapshot", MeasureUnit::MICROSECOND);

    double best_bid = snapshot->get_best_bid();
    double best_ask = snapshot->get_best_ask();
    double mid = (best_bid + best_ask) / 2.0;
    spdlog::debug("StrategyMarketMakerStateRun - best_bid: {}, best_ask: {}, mid: {}", best_bid, best_ask, mid);

    if (std::abs(mid - m_last_quote_price) > m_config.price_step_between_blocks)
    {
        quote_block_orders_at_price(mid);
        m_last_quote_price = mid;
    }
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
            m_inventory += 1;
        }
        else if (order.side == Order::Side::SELL)
        {
            m_inventory -= 1;
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