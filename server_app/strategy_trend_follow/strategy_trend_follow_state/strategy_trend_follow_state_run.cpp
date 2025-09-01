#include "strategy_trend_follow_state_run.h"
#include <time/measure_time.h>
#include <utils/utils.h>
#include <enum_reflect/enum_reflect.h>

StrategyTrendFollowStateRun::StrategyTrendFollowStateRun(std::shared_ptr<Gateway> gateway, const StrategyTrendFollowConfig& config)
    : m_gateway{gateway}, m_config{config}, m_event_base{EventBaseManager::get_event_base_by_id(EventBaseID::TREND_FOLLOW_STRATEGY)}
{
}

void StrategyTrendFollowStateRun::begin()
{
    on_config_change();
    spdlog::info("StrategyTrendFollowStateRun - begin");
}

void StrategyTrendFollowStateRun::end()
{
    spdlog::info("StrategyTrendFollowStateRun - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_instrument->exchange_symbol);
}

void StrategyTrendFollowStateRun::on_config_change()
{
    m_instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
}

Json StrategyTrendFollowStateRun::get_info()
{
    return {};
}

Order StrategyTrendFollowStateRun::get_limit_order(Order::Side side, double price, double quantity)
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

void StrategyTrendFollowStateRun::handle_price_update(PriceUpdate price_update)
{
    MeasureTime t("StrategyTrendFollowStateRun - handle_price_update");
}

void StrategyTrendFollowStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    double best_bid = snapshot->get_best_bid();
    double best_ask = snapshot->get_best_ask();
    double mid = (best_bid + best_ask) / 2.0;

    m_price_gap = best_ask - best_bid;
    if (m_price_gap >= m_config.price_gap)
    {
        spdlog::info("StrategyTrendFollowStateRun - Price gap {} is larger than configured {}, spamming orders", m_price_gap, m_config.price_gap);
        Order::Side side = (mid > m_last_price) ? Order::Side::BUY : Order::Side::SELL;
        m_is_pump = (side == Order::Side::BUY);

        // Spam orders between the price gap, hope it will get filled
        double price = best_bid + m_config.price_step;
        while (price < best_ask)
        {
            Order order = get_limit_order(side, price, m_config.volumn);
            m_gateway->place(order);

            spdlog::info("StrategyTrendFollowStateRun - Spamming order: side={}, price={}, quantity={}", enum_reflect::enum_name<Order::Side>(order.side), order.price, order.quantity);

            price += m_config.price_step;
        }
    }

    m_last_price = mid;
}

void StrategyTrendFollowStateRun::handle_order_update(Order& order)
{
    MeasureTime t("StrategyTrendFollowStateRun - handle_order_update");

    // NEW - add to [m_open_orders]
    if (order.status == Order::Status::NEW)
    {
    }
    // FILLED - update [m_open_bid_orders] or [m_open_ask_orders]
    else if (order.status == Order::Status::FILLED)
    {
        if (order.side == Order::Side::BUY && m_is_pump == true)
        {
            Order order = get_limit_order(Order::Side::SELL, order.price + m_config.take_profit, order.quantity);
            m_gateway->place(order);
            spdlog::info("StrategyTrendFollowStateRun - Placing take profit SELL order, side: {}, price: {}, quantity: {}", enum_reflect::enum_name<Order::Side>(order.side), order.price, order.quantity);
        }
        else if (order.side == Order::Side::SELL && m_is_pump == false)
        {
            Order order = get_limit_order(Order::Side::BUY, order.price - m_config.take_profit, order.quantity);
            m_gateway->place(order);
            spdlog::info("StrategyTrendFollowStateRun - Placing take profit BUY order, side: {}, price: {}, quantity: {}", enum_reflect::enum_name<Order::Side>(order.side), order.price, order.quantity);
        }
    }
    // CANCELED or REJECTED - do nothing
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
    }
}

Task<void> StrategyTrendFollowStateRun::update(StrategyUpdateData data)
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