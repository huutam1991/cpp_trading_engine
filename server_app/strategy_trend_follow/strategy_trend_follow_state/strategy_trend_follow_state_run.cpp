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
    m_inventory = 0.0;
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
    double best_bid_price = snapshot->get_best_bid();
    double best_ask_price = snapshot->get_best_ask();
    double best_bid_quantity = snapshot->get_best_bid_quantity();
    double best_ask_quantity = snapshot->get_best_ask_quantity();

    if (best_bid_quantity / best_ask_quantity > m_config.ratio)
    {
        Order order = get_limit_order(Order::Side::BUY, best_bid_price + m_config.price_step, m_config.volume);
        m_gateway->place(order);
    }
    else if (best_ask_quantity / best_bid_quantity > m_config.ratio)
    {
        Order order = get_limit_order(Order::Side::SELL, best_ask_price - m_config.price_step, m_config.volume);
        m_gateway->place(order);
    }
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
        m_inventory += order.side == Order::Side::BUY ? 1 : -1;

        if (order.side == Order::Side::BUY && m_inventory > 0)
        {
            Order order = get_limit_order(Order::Side::SELL, order.price + m_config.take_profit, order.quantity);
            spdlog::info("StrategyTrendFollowStateRun - Placing take profit SELL order, side: {}, price: {}, quantity: {}", enum_reflect::enum_name<Order::Side>(order.side), order.price, order.quantity);
            m_gateway->place(order);
        }
        else if (order.side == Order::Side::SELL && m_inventory < 0)
        {
            Order order = get_limit_order(Order::Side::BUY, order.price - m_config.take_profit, order.quantity);
            spdlog::info("StrategyTrendFollowStateRun - Placing take profit BUY order, side: {}, price: {}, quantity: {}", enum_reflect::enum_name<Order::Side>(order.side), order.price, order.quantity);
            m_gateway->place(order);
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