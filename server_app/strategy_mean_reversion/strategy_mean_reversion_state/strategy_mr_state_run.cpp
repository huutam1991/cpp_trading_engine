#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_run.h>
#include <time/measure_time.h>

StrategyMeanReversionStateRun::StrategyMeanReversionStateRun(std::shared_ptr<Gateway> gateway, const StrategyMeanReversionConfig& config)
    : m_gateway{gateway}, m_config{config}
{
    m_instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
}

void StrategyMeanReversionStateRun::begin()
{
    m_current_price = 0.0;
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
}

void StrategyMeanReversionStateRun::handle_trade_update(TradeUpdate& trade_update)
{
}

void StrategyMeanReversionStateRun::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMarketMakerStateRun - handle_order_book_snapshot", MeasureUnit::MICROSECOND);
    m_current_price = snapshot->get_mid_price();

    if (m_buy_order == nullptr && m_sell_order == nullptr)
    {
        // double buy_price = m_current_price - 10;
        // double sell_price = m_current_price + 10;
        // double quantity = 1.0;

        // m_buy_order = get_limit_order(Order::Side::BUY, buy_price, quantity);
        // m_sell_order = get_limit_order(Order::Side::SELL, sell_price, quantity);

        // m_gateway->place(m_buy_order);
        // m_gateway->place(m_sell_order);
    }
}

void StrategyMeanReversionStateRun::handle_order_update(Order& order)
{
}