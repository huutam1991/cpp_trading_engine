#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_stop.h>

StrategyMeanReversionStateStop::StrategyMeanReversionStateStop(
    std::shared_ptr<Gateway> gateway,
    const StrategyMeanReversionConfig& config,
    std::vector<SpreadCaptureConfig>& spread_captures)
    : m_gateway{gateway}, m_config{config}, m_spread_captures{spread_captures}
{
}

void StrategyMeanReversionStateStop::begin()
{
    spdlog::info("StrategyMeanReversionStateStop - begin");
}

void StrategyMeanReversionStateStop::end()
{
    spdlog::info("StrategyMeanReversionStateStop - end");
}

Json StrategyMeanReversionStateStop::get_info()
{
    Json spread_captures;

    for (auto& spread_capture : m_spread_captures)
    {
        spread_captures.push_back({
            {"entry_distance", spread_capture.entry_distance},
            {"take_profit", spread_capture.take_profit},
            {"stop_loss", spread_capture.stop_loss},
            {"success", spread_capture.success},
            {"fail", spread_capture.fail},
            {"win_rate", spread_capture.win_rate()},
            {
                "current_status", {
                    {"current_price", m_current_price},
                    {"status", enum_reflect::enum_name(spread_capture.status)},
                    {"buy_order", {
                        {"price", spread_capture.buy_order.price},
                        {"status", enum_reflect::enum_name(spread_capture.buy_order.status)}
                    }},
                    {"sell_order", {
                        {"price", spread_capture.sell_order.price},
                        {"status", enum_reflect::enum_name(spread_capture.sell_order.status)}
                    }}
                }
            }
        });
    }

    return spread_captures;
}

void StrategyMeanReversionStateStop::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMeanReversionStateStop - handle_order_book_snapshot", MeasureUnit::MICROSECOND);
    m_current_price = snapshot->get_mid_price();

    for (auto& spread_capture : m_spread_captures)
    {
        if (spread_capture.status == SpreadCaptureConfig::Status::NONE)
        {
            spread_capture.buy_order.status = Order::Status::NEW;
            spread_capture.buy_order.price = m_current_price - spread_capture.entry_distance;
            spread_capture.sell_order.status = Order::Status::NEW;
            spread_capture.sell_order.price = m_current_price + spread_capture.entry_distance;

            spread_capture.status = SpreadCaptureConfig::Status::PLACING_INIT_ORDERS;
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_INIT_ORDERS)
        {
            if (m_current_price < spread_capture.buy_order.price)
            {
                spread_capture.buy_order.status = Order::Status::FILLED;
                spread_capture.status = SpreadCaptureConfig::Status::PLACING_HEDGE_SELL_ORDER;
                spread_capture.sell_order.price = m_current_price + spread_capture.take_profit;
            }
            else if (m_current_price > spread_capture.sell_order.price)
            {
                spread_capture.sell_order.status = Order::Status::FILLED;
                spread_capture.status = SpreadCaptureConfig::Status::PLACING_HEDGE_BUY_ORDER;
                spread_capture.buy_order.price = m_current_price - spread_capture.take_profit;
            }
            else
            {
                // Update new order price
                spread_capture.buy_order.price = m_current_price - spread_capture.entry_distance;
                spread_capture.sell_order.price = m_current_price + spread_capture.entry_distance;
            }
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_BUY_ORDER)
        {
            if (m_current_price < spread_capture.buy_order.price)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.success++;
            }
            else if (m_current_price >= spread_capture.sell_order.price + spread_capture.stop_loss)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.fail++;
            }
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_SELL_ORDER)
        {
            if (m_current_price > spread_capture.sell_order.price)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.success++;
            }
            else if (m_current_price <= spread_capture.buy_order.price - spread_capture.stop_loss)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.fail++;
            }
        }
    }
}

Task<void> StrategyMeanReversionStateStop::update(StrategyUpdateData data)
{
    PriceUpdate price_update;
    if (std::holds_alternative<PriceUpdate>(data))
    {
        price_update = std::get<PriceUpdate>(data);
        spdlog::debug("StrategyMeanReversionStateStop: do nothing, symbol: {}, price: {}", price_update.instrument->symbol, price_update.price);
    }
    else if (std::holds_alternative<OrderBookSnapShot*>(data))
    {
        OrderBookSnapShot* snapshot = std::get<OrderBookSnapShot*>(data);
        handle_order_book_snapshot(snapshot);

        OrderBookSnapShotPool::release(snapshot);
    }

    co_return;
}