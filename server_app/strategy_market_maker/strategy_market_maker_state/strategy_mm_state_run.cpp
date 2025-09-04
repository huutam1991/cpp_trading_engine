#include "strategy_mm_state_run.h"
#include <time/measure_time.h>
#include <time/timer.h>
#include <utils/utils.h>
#include <enum_reflect/enum_reflect.h>

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

    m_inventory = 0.0;
    m_current_price = 0.0;
    m_place_initial_orders = false;
    m_open_orders.clear();

    // Send cancel all of placed order
    m_gateway->cancel_all(m_instrument->exchange_symbol);
}

void StrategyMarketMakerStateRun::on_config_change()
{
    m_instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
    start_close_far_orders();
}

Json StrategyMarketMakerStateRun::get_info()
{
    Json open_orders;

    for (const auto& [order_id, order] : m_open_orders)
    {
        Json data = order.to_json();
        double distance = std::abs(order.price - m_current_price);
        data["distance"] = distance;
        data["is_far_order"] = (distance > m_config.clear_orders_gap);
        open_orders.push_back(data);
    }

    return {
        {"open_orders", open_orders},
        {"current_price", m_current_price},
        {"inventory", m_inventory},
    };
}

Order StrategyMarketMakerStateRun::get_limit_order(Order::Side side, double price, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        side,
        Order::OrderType::LIMIT,
        m_instrument->get_round_up_price(price),
        quantity
    );
}

void StrategyMarketMakerStateRun::start_close_far_orders()
{
    if (m_is_closing_far_orders == false && m_config.is_running == true)
    {
        auto task = task_close_far_orders();
        task.start_running_on(m_event_base);
    }
}

Task<void> StrategyMarketMakerStateRun::task_close_far_orders()
{
    spdlog::warn("task_close_far_orders, m_open_orders size: {}", static_cast<uint64_t>(m_open_orders.size()));
    m_is_closing_far_orders = true;

    for (auto& [order_id, order] : m_open_orders)
    {
        double price_distance = std::abs(order.price - m_current_price);
        spdlog::warn("task_close_far_orders, order: {}, price_distance: {}, clear_orders_gap: {}", order.to_json(), price_distance, m_config.clear_orders_gap);
        if (price_distance > m_config.clear_orders_gap)
        {
            spdlog::info("task_close_far_orders, clear_orders_gap: cancel order at price: {}, distance: {}, price: {}", order.price, price_distance, m_current_price);
            m_gateway->cancel(order);
        }
    }

    // Check every 30 seconds
    co_await Timer::sleep_for(30000);
    m_is_closing_far_orders = false;
    start_close_far_orders();

    co_return;
}

void StrategyMarketMakerStateRun::quote_orders_at_price(double price)
{
    MeasureTime t("StrategyMarketMakerStateRun - quote_orders_at_price");

    double buy_price = price - m_config.price_gap - m_inventory * m_config.price_step;
    double sell_price = price + m_config.price_gap - m_inventory * m_config.price_step;

    buy_price = std::min(buy_price, price - 1.0);
    sell_price = std::max(sell_price, price + 1.0);

    Order buy_order  = get_limit_order(Order::Side::BUY, buy_price, m_config.volumn);
    Order sell_order = get_limit_order(Order::Side::SELL, sell_price, m_config.volumn);

    m_gateway->place(buy_order);
    m_gateway->place(sell_order);

    spdlog::info("StrategyMarketMakerStateRun - quote_orders_at_price: {}, place buy order at price: {}, sell order at price: {}, inventory: {}",
        price,
        buy_price,
        sell_price,
        m_inventory
    );
}

void StrategyMarketMakerStateRun::handle_price_update(PriceUpdate price_update)
{
    MeasureTime t("StrategyMarketMakerStateRun - handle_price_update");
}

void StrategyMarketMakerStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    double best_bid = snapshot->get_best_bid();
    double best_ask = snapshot->get_best_ask();
    double mid = (best_bid + best_ask) / 2.0;

    m_current_price = mid;

    // If there's no open orders, quote new orders
    if (m_open_orders.size() == 0 && m_place_initial_orders == false)
    {
        quote_orders_at_price(mid);
        m_place_initial_orders = true;
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

        quote_orders_at_price(order.price);
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