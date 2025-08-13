#include "strategy_trend_follow_state_run.h"
#include <time/measure_time.h>
#include <utils/utils.h>
#include <enum_reflect/enum_reflect.h>

StrategyTrendFollowStateRun::StrategyTrendFollowStateRun(std::shared_ptr<Gateway> gateway, const StrategyTrendFollowConfig& config)
    : m_gateway{gateway}, m_config{config}, m_event_base{EventBaseManager::get_event_base_by_id(EventBaseID::MARKET_MAKER_STRATEGY)}
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

Order StrategyTrendFollowStateRun::get_buy_limit_order(double price, double quantity)
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

Order StrategyTrendFollowStateRun::get_sell_limit_order(double price, double quantity)
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

void StrategyTrendFollowStateRun::handle_price_update(PriceUpdate price_update)
{
    MeasureTime t("StrategyTrendFollowStateRun - handle_price_update");
}

void StrategyTrendFollowStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    double best_bid = snapshot->get_best_bid();
    double best_ask = snapshot->get_best_ask();
    double mid = (best_bid + best_ask) / 2.0;
}


void StrategyTrendFollowStateRun::handle_order_update(Order& order)
{
    MeasureTime t("StrategyTrendFollowStateRun - handle_order_update");

    // NEW - add to [m_open_orders]
    if (order.status == Order::Status::NEW)
    {
        if (order.side == Order::Side::BUY)
        {
            m_open_bid_orders.emplace(order.order_id, order);
        }
        else if (order.side == Order::Side::SELL)
        {
            m_open_ask_orders.emplace(order.order_id, order);
        }
    }
    // FILLED - update [m_open_bid_orders] or [m_open_ask_orders]
    else if (order.status == Order::Status::FILLED)
    {
        if (order.side == Order::Side::BUY)
        {
            m_open_bid_orders[order.order_id].status = order.status;
        }
        else if (order.side == Order::Side::SELL)
        {
            m_open_ask_orders[order.order_id].status = order.status;
        }
    }
    // CANCELED or REJECTED - remove from [m_open_orders]
    else if (order.status == Order::Status::CANCELED || order.status == Order::Status::REJECTED)
    {
        if (order.side == Order::Side::BUY)
        {
            m_open_bid_orders.erase(order.order_id);
        }
        else if (order.side == Order::Side::SELL)
        {
            m_open_ask_orders.erase(order.order_id);
        }
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