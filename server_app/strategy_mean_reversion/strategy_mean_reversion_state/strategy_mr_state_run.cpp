#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_run.h>
#include <time/measure_time.h>

StrategyMeanReversionStateRun::StrategyMeanReversionStateRun(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config, SpreadCaptureConfigManager& spread_captures)
    : m_gateway{gateway}, m_config{config}, m_spread_captures{spread_captures}
{
    m_instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
    m_pnl.update_instrument(m_instrument);
}

void StrategyMeanReversionStateRun::begin()
{
    spdlog::info("StrategyMeanReversionStateRun - begin");

    m_current_price = 0.0;
    m_pnl.reset();
    m_spread_captures.reset();
}

void StrategyMeanReversionStateRun::end()
{
    spdlog::info("StrategyMeanReversionStateRun - end");

    m_pnl.reset();
    // Send cancel all of placed order
    m_config.account->m_order_entry->cancel_all(m_instrument->exchange_symbol);
}

Json StrategyMeanReversionStateRun::get_info()
{
    return {
        {"spread_captures", m_spread_captures.get_info()},
        {"current_price", m_current_price},
        {"pnl", m_pnl.get_data()}
    };
}

bool StrategyMeanReversionStateRun::is_same_order_info(Order& order1, Order& order2)
{
    return std::abs(order1.price - order2.price) <= m_instrument->price_precision &&
        order1.side == order2.side &&
        order1.type == order2.type;
}

Order StrategyMeanReversionStateRun::get_limit_order(Order::Side side, double price, double quantity)
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

Order StrategyMeanReversionStateRun::get_market_order(Order::Side side, double quantity)
{
    return Order(
        OrderManager::instance().generate_order_id(),
        Order::Status::NOT_AVAILABLE,
        m_instrument,
        side,
        Order::OrderType::MARKET,
        0.0,
        quantity
    );
}

void StrategyMeanReversionStateRun::handle_price_update(PriceUpdate& price_update)
{
    m_current_price = price_update.price;
    m_pnl.update_current_price(m_current_price);
}

void StrategyMeanReversionStateRun::handle_trade_update(TradeUpdate& trade_update)
{
}

void StrategyMeanReversionStateRun::handle_order_update(Order& order)
{
    PipelineTraceBuffer::RecordStageTiming<PipelineStage::STRATEGY_UPDATE> record_stage(order.trace_id);

    m_spread_captures.handle_order_update(order);

    if (order.status == Order::Status::NEW)
    {
        if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER)
        {
            m_initial_order = order;
        }
        else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_ORDER ||
                 m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::WAITING_FOR_HEDGE_ORDER_FILLED ||
                 m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS)
        {
            m_hedge_order = order;
        }
    }
    else if (order.status == Order::Status::REJECTED)
    {
        // Re-place order if it's rejected
        if (order.side == Order::Side::BUY)
        {
            // Try to place buy order again with lower price
            order.price = m_current_price - 0.5;
            order.side = Order::Side::BUY;
        }
        else if (order.side == Order::Side::SELL)
        {
            // Try to place sell order again with higher price
            order.price = m_current_price + 0.5;
            order.side = Order::Side::SELL;
        }

        Order new_order = get_limit_order(order.side, order.price, m_config.volume);
        new_order.trace_id = order.trace_id;
        m_config.account->m_order_entry->place(new_order);
    }
    else if (order.status == Order::Status::FILLED)
    {
        double trade_volume = (order.side == Order::Side::BUY) ? order.filled_quantity : -order.filled_quantity;
        m_pnl.update_trade(order.filled_price, trade_volume, order.fee);

        if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER)
        {
            m_initial_order = order;
        }
        else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_ORDER ||
                 m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::WAITING_FOR_HEDGE_ORDER_FILLED ||
                 m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS)
        {
            m_hedge_order = order;
        }
    }
    // Place market order after Limit order is canceled due to stop loss
    else if (order.status == Order::Status::CANCELED)
    {
        if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER)
        {
            Order& initial_order = m_spread_captures.spread_capture.initial_order;
            m_initial_order = get_limit_order(initial_order.side, initial_order.price, m_config.volume);

            // Re-place

            m_initial_order.trace_id = order.trace_id;
            m_config.account->m_order_entry->place(m_initial_order);
        }
        else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS)
        {
            if (order.side == Order::Side::BUY)
            {
                order.price = m_current_price - 0.5;
            }
            else if (order.side == Order::Side::SELL)
            {
                order.price = m_current_price + 0.5;
            }

            m_hedge_order = get_limit_order(order.side, order.price, m_config.volume);
            m_hedge_order.trace_id = order.trace_id;
            m_config.account->m_order_entry->place(m_hedge_order);
        }
    }
}

void StrategyMeanReversionStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    PipelineTraceBuffer::RecordStageTiming<PipelineStage::STRATEGY_UPDATE> record_stage(snapshot->trace_id);

    m_current_price = snapshot->get_mid_price();
    m_pnl.update_current_price(m_current_price);

    m_spread_captures.handle_order_book_snapshot(snapshot);

    if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER)
    {
        if (m_initial_order == nullptr)
        {
            m_initial_order = get_limit_order(Order::Side::BUY, m_spread_captures.spread_capture.initial_order.price, m_config.volume);
            m_initial_order.trace_id = snapshot->trace_id;
            m_config.account->m_order_entry->place(m_initial_order);
        }
        else if (m_initial_order.status == Order::Status::NEW &&
                is_same_order_info(m_initial_order, m_spread_captures.spread_capture.initial_order) == false)
        {
            m_initial_order.trace_id = snapshot->trace_id;
            m_config.account->m_order_entry->cancel(m_initial_order);
            m_initial_order.status = Order::Status::NOT_AVAILABLE;
        }
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_ORDER)
    {
        Order& hedge_order = m_spread_captures.spread_capture.hedge_order;
        m_hedge_order = get_limit_order(hedge_order.side, hedge_order.price, m_config.volume);
        m_hedge_order.trace_id = snapshot->trace_id;
        m_config.account->m_order_entry->place(m_hedge_order);
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::WAITING_FOR_HEDGE_ORDER_FILLED)
    {
        if (m_hedge_order.status == Order::Status::FILLED)
        {
            m_initial_order = nullptr;
            m_hedge_order = nullptr;
        }
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS)
    {
        if (m_hedge_order.status == Order::Status::NEW)
        {
            m_hedge_order.trace_id = snapshot->trace_id;
            m_config.account->m_order_entry->cancel(m_hedge_order);
            m_hedge_order.status = Order::Status::NOT_AVAILABLE;
        }
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::NONE)
    {
        m_initial_order = nullptr;
        m_hedge_order = nullptr;
    }
}