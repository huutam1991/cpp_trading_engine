#include "strategy_mm_state_run.h"
#include <time/measure_time.h>
#include <utils/utils.h>

StrategyMarketMakerStateRun::StrategyMarketMakerStateRun(std::shared_ptr<Gateway> gateway, const StrategyMarketMakerConfig& config)
    : m_gateway{gateway}, m_config{config}, m_event_base{EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY)}
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

double StrategyMarketMakerStateRun::get_inventory()
{
    return m_inventory;
}

Order StrategyMarketMakerStateRun::get_buy_limit_order(double price, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        Order::Side::BUY,
        Order::OrderType::LIMIT,
        m_instrument->get_round_up_price(price),
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
        m_instrument->get_round_up_price(price),
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

void StrategyMarketMakerStateRun::close_far_orders(double price)
{
    auto task = task_close_far_orders(price);
    task.start_running_on(m_event_base);
}

Task<void> StrategyMarketMakerStateRun::task_close_far_orders(double price)
{
    spdlog::warn("m_open_orders size: {}", static_cast<uint64_t>(m_open_orders.size()));
    for (auto& [order_id, order] : m_open_orders)
    {
        spdlog::warn("order: {}", order.to_json());
        double price_distance = std::abs(order.price - price);
        if (price_distance > m_config.price_step_between_blocks)
        {
            spdlog::info("StrategyMarketMakerStateRun - close_far_orders: cancel order at price: {}, distance: {}", order.price, price_distance);
            m_gateway->cancel(order);
        }
    }

    co_return;
}

void StrategyMarketMakerStateRun::quote_block_orders_at_price(double price)
{
    MeasureTime t("StrategyMarketMakerStateRun - quote_block", MeasureUnit::MICROSECOND);

    // Calculate the price for buy and sell orders
    double inventory_in_blocks = std::abs(m_inventory / m_config.orders_each_side_per_block);

    double alpha = inventory_in_blocks >= m_config.inventory_skew_ratio ? 1.0 : 0.0;
    double widen = 1.0 + (m_config.widen - 1.0) * alpha;
    double tighten = 1.0 - (1.0 - m_config.tight) * alpha;

    double bid_price_gap = m_config.price_gap;
    double ask_price_gap = m_config.price_gap;
    if (m_inventory > 0)
    {
        // Inventory is positive, widen the bid price
        bid_price_gap = m_config.price_gap * widen;
        ask_price_gap = m_config.price_gap * tighten;
    }
    else if (m_inventory < 0)
    {
        // Inventory is negative, widen the ask price
        bid_price_gap = m_config.price_gap * tighten;
        ask_price_gap = m_config.price_gap * widen;
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
        close_far_orders(mid);

        m_last_quote_price = mid;
    }
}

void StrategyMarketMakerStateRun::handle_order_update(Order& order)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_order_update");

    // NEW - add to [m_open_orders]
    if (order.status == Order::Status::NEW)
    {
        m_open_orders.emplace(order.order_id, order);
    }
    // FILLED - update [m_inventory] and remove order from [m_open_orders]
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

        m_open_orders.erase(order.order_id);
    }
    // CANCELED or REJECTED - remove from [m_open_orders]
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        m_open_orders.erase(order.order_id);
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