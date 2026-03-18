#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_run.h>
#include <time/measure_time.h>

StrategyMeanReversionStateRun::StrategyMeanReversionStateRun(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config, SpreadCaptureConfigManager& spread_captures)
    : m_gateway{gateway}, m_config{config}, m_spread_captures{spread_captures}
{
    m_instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
}

void StrategyMeanReversionStateRun::begin()
{
    m_current_price = 0.0;
    m_pnl.reset();
    spdlog::info("StrategyMeanReversionStateRun - begin");
}

void StrategyMeanReversionStateRun::end()
{
    spdlog::info("StrategyMeanReversionStateRun - end");

    // Send cancel all of placed order
    m_gateway->cancel_all(m_config.symbol);
}

Json StrategyMeanReversionStateRun::get_info()
{
    return {
        {"info", "TBD"}
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

    for (auto& spread_capture : m_spread_captures.spread_captures)
    {
        if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_ORDERS &&
            m_buy_order == nullptr && m_sell_order == nullptr)
        {
            m_buy_order  = get_limit_order(Order::Side::BUY, spread_capture.buy_order.price, 1.0);
            m_sell_order = get_limit_order(Order::Side::SELL, spread_capture.sell_order.price, 1.0);

            m_gateway->place(m_buy_order);
            m_gateway->place(m_sell_order);
        }
    }
}

void StrategyMeanReversionStateRun::handle_order_update(Order& order)
{
    if (order.status == Order::Status::NEW)
    {
        m_spread_captures.handle_order_update(order);

        // Update orders
        if (order.side == Order::Side::BUY)
        {
            m_buy_order = nullptr;
        }
        else if (order.side == Order::Side::SELL)
        {
            m_sell_order = nullptr;
        }
    }
    else if (order.status == Order::Status::FILLED)
    {
        double trade_volume = (order.side == Order::Side::BUY) ? order.filled_quantity : -order.filled_quantity;
        m_pnl.update_trade(order.filled_price, trade_volume, order.fee);
    }
}