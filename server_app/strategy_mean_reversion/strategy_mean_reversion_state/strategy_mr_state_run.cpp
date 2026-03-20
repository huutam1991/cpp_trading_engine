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
    m_gateway->cancel_all(m_instrument->exchange_symbol);
}

Json StrategyMeanReversionStateRun::get_info()
{
    return {
        {"spread_captures", m_spread_captures.get_info()},
        {"current_price", m_current_price},
        {"pnl", m_pnl.get_data()}
    };
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

void StrategyMeanReversionStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMarketMakerStateRun - handle_order_book_snapshot", MeasureUnit::MICROSECOND);
    m_current_price = snapshot->get_mid_price();
    m_pnl.update_current_price(m_current_price);

    m_spread_captures.handle_order_book_snapshot(snapshot);

    if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_BUY_INITIAL_ORDER)
    {
        if (m_buy_order == nullptr)
        {
            m_buy_order  = get_limit_order(Order::Side::BUY, m_spread_captures.spread_capture.buy_order.price, m_config.volume);
            m_gateway->place(m_buy_order);
        }
        else if (m_buy_order.status == Order::Status::NEW)
        {
            // Cancel and re-place with more close price if current price moves away from buy order price too much
            if (m_current_price - m_buy_order.price > 0.5)
            {
                m_gateway->cancel(m_buy_order);
                m_buy_order.status = Order::Status::NOT_AVAILABLE;
            }
        }
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_SELL_INITIAL_ORDER)
    {
        if (m_sell_order == nullptr)
        {
            m_sell_order = get_limit_order(Order::Side::SELL, m_spread_captures.spread_capture.sell_order.price, m_config.volume);
            m_gateway->place(m_sell_order);
        }
        else if (m_sell_order.status == Order::Status::NEW)
        {
            // Cancel and re-place with more close price if current price moves away from sell order price too much
            if (m_sell_order.price - m_current_price > 0.5)
            {
                m_gateway->cancel(m_sell_order);
                m_sell_order.status = Order::Status::NOT_AVAILABLE;
            }
        }
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_BUY_HEDGE_ORDER &&
        m_buy_order == nullptr)
    {
        m_buy_order  = get_limit_order(Order::Side::BUY, m_spread_captures.spread_capture.buy_order.price, m_config.volume);
        m_gateway->place(m_buy_order);
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::PLACING_SELL_HEDGE_ORDER &&
        m_sell_order == nullptr)
    {
        m_sell_order = get_limit_order(Order::Side::SELL, m_spread_captures.spread_capture.sell_order.price, m_config.volume);
        m_gateway->place(m_sell_order);
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS_BUY)
    {
        if (m_sell_order != nullptr && m_sell_order.status == Order::Status::NEW)
        {
            m_gateway->cancel(m_sell_order);
            m_sell_order = nullptr;
        }
    }
    else if (m_spread_captures.spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS_SELL)
    {
        if (m_buy_order != nullptr && m_buy_order.status == Order::Status::NEW)
        {
            m_gateway->cancel(m_buy_order);
            m_buy_order = nullptr;
        }
    }
}

void StrategyMeanReversionStateRun::handle_order_update(Order& order)
{
    m_spread_captures.handle_order_update(order);

    if (order.status == Order::Status::NEW)
    {
        if (order.side == Order::Side::BUY)
        {
            m_buy_order = order;
        }
        else if (order.side == Order::Side::SELL)
        {
            m_sell_order = order;
        }
    }
    else if (order.status == Order::Status::REJECTED)
    {
        // Re-place order if it's rejected
        if (order.side == Order::Side::BUY)
        {
            m_buy_order = get_limit_order(Order::Side::BUY, m_spread_captures.spread_capture.buy_order.price, m_config.volume);
            m_gateway->place(m_buy_order);
        }
        else if (order.side == Order::Side::SELL)
        {
            m_sell_order = get_limit_order(Order::Side::SELL, m_spread_captures.spread_capture.sell_order.price, m_config.volume);
            m_gateway->place(m_sell_order);
        }
    }
    else if (order.status == Order::Status::FILLED)
    {
        double trade_volume = (order.side == Order::Side::BUY) ? order.filled_quantity : -order.filled_quantity;
        m_pnl.update_trade(order.filled_price, trade_volume, order.fee);

        if (order.side == Order::Side::BUY)
        {
            m_buy_order = nullptr;
        }
        else if (order.side == Order::Side::SELL)
        {
            m_sell_order = nullptr;
        }
    }
    // Place market order after Limit order is canceled due to stop loss
    else if (order.status == Order::Status::CANCELED)
    {
        if (order.side == Order::Side::BUY)
        {
            m_buy_order = get_limit_order(Order::Side::BUY, m_spread_captures.spread_capture.buy_order.price, m_config.volume);
            m_gateway->place(m_buy_order);
        }
        else if (order.side == Order::Side::SELL)
        {
            m_sell_order = get_limit_order(Order::Side::SELL, m_spread_captures.spread_capture.sell_order.price, m_config.volume);
            m_gateway->place(m_sell_order);
        }
    }
}