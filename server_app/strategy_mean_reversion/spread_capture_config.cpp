#include "spread_capture_config.h"

void SpreadCaptureConfigManager::init_from_config(const SpreadCaptureConfig& config)
{
    spread_capture = config;
}

void SpreadCaptureConfigManager::reset()
{
    spread_capture.reset();
    volatility_estimator.reset();
}

Json SpreadCaptureConfigManager::get_info()
{
    Json data;

    data.push_back({
        {"move_distance", spread_capture.move_distance},
        {"entry_distance", spread_capture.entry_distance},
        {"take_profit", spread_capture.take_profit},
        {"stop_loss", spread_capture.stop_loss},
        {"success", spread_capture.success},
        {"fail", spread_capture.fail},
        {"win_rate", spread_capture.win_rate()},
        {"pnl", {
            {"profit", spread_capture.profit},
            {"loss", spread_capture.loss},
            {"pnl", spread_capture.profit + spread_capture.loss}
        }},
        {
            "current_status", {
                {"mean_price", spread_capture.mean_price},
                {"volatility", spread_capture.volatility},
                {"status", enum_reflect::enum_name(spread_capture.status)},
                {"current_configs", {
                    {"current_move_distance", spread_capture.current_move_distance},
                    {"current_entry_distance", spread_capture.current_entry_distance},
                    {"current_take_profit", spread_capture.current_take_profit},
                    {"current_stop_loss", spread_capture.current_stop_loss},
                }},
                {"orders", {
                    {"buy_order", {
                        {"price", spread_capture.initial_order.price},
                        {"status", enum_reflect::enum_name(spread_capture.initial_order.status)}
                    }},
                    {"sell_order", {
                        {"price", spread_capture.initial_order.price},
                        {"status", enum_reflect::enum_name(spread_capture.initial_order.status)}
                    }}
                }},
            }
        }
    });

    return data;
}

void SpreadCaptureConfigManager::handle_order_update(Order& order)
{
    if (order.status == Order::Status::NEW)
    {
        if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER)
        {
            spread_capture.initial_order = order;
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_ORDER ||
                 spread_capture.status == SpreadCaptureConfig::Status::WAITING_FOR_HEDGE_ORDER_FILLED ||
                 spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS)
        {
            spread_capture.hedge_order = order;
        }
    }
    else if (order.status == Order::Status::REJECTED)
    {
        // TBD
    }
    else if (order.status == Order::Status::FILLED)
    {
        if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER)
        {
            spread_capture.initial_order = order;
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_ORDER ||
                 spread_capture.status == SpreadCaptureConfig::Status::WAITING_FOR_HEDGE_ORDER_FILLED ||
                 spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS)
        {
            spread_capture.hedge_order = order;
        }
    }
    // Order is canceled due to stop loss, need to place stop loss order
    else if (order.status == Order::Status::CANCELED)
    {
        // TBD
    }
}

void SpreadCaptureConfigManager::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMeanReversionStateStop - handle_order_book_snapshot", MeasureUnit::MICROSECOND);
    double mid_price = snapshot->get_mid_price();
    volatility_estimator.update(mid_price);

    double volatility = volatility_estimator.stddev();
    spread_capture.volatility = volatility;
    spread_capture.mean_price = volatility_estimator.mean();

    if (spread_capture.mean_price == 0.0)
    {
        return; // Not enough data to calculate mean price, do nothing
    }

    double prev_price = volatility_estimator.get_prev_price();

    if (spread_capture.status == SpreadCaptureConfig::Status::NONE ||
        spread_capture.status == SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER)
    {
        spread_capture.current_move_distance = volatility * spread_capture.move_distance;
        spread_capture.current_entry_distance = volatility * spread_capture.entry_distance;
        spread_capture.current_take_profit = volatility * spread_capture.take_profit;
        spread_capture.current_stop_loss = volatility * spread_capture.stop_loss;

        // Hedge order in the beginning is not placed yet
        spread_capture.hedge_order = nullptr;

        if (spread_capture.initial_order.status != Order::Status::FILLED)
        {
            if (mid_price > spread_capture.mean_price)
            {
                // Try to sell first then buy back
                spread_capture.initial_order.status = Order::Status::NOT_AVAILABLE;
                spread_capture.initial_order.price = mid_price + spread_capture.current_move_distance;
                spread_capture.initial_order.side = Order::Side::SELL;
            }
            else
            {
                // Try to buy first then sell back
                spread_capture.initial_order.status = Order::Status::NOT_AVAILABLE;
                spread_capture.initial_order.price = mid_price - spread_capture.current_move_distance;
                spread_capture.initial_order.side = Order::Side::BUY;
            }
        }
        // Update status to PLACING_HEDGE_ORDER
        else
        {
            if (spread_capture.initial_order.side == Order::Side::BUY)
            {
                spread_capture.hedge_order.side = Order::Side::SELL;
                spread_capture.hedge_order.price = spread_capture.initial_order.price + spread_capture.current_take_profit;
                spread_capture.hedge_order.status = Order::Status::NOT_AVAILABLE;
            }
            else if (spread_capture.initial_order.side == Order::Side::SELL)
            {
                spread_capture.hedge_order.side = Order::Side::BUY;
                spread_capture.hedge_order.status = Order::Status::NOT_AVAILABLE;
                spread_capture.hedge_order.price = spread_capture.initial_order.price - spread_capture.current_take_profit;
            }

            spread_capture.status = SpreadCaptureConfig::Status::PLACING_HEDGE_ORDER;
        }
    }
    else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_ORDER)
    {
        if (spread_capture.hedge_order.status == Order::Status::NEW)
        {
            spread_capture.status = SpreadCaptureConfig::Status::WAITING_FOR_HEDGE_ORDER_FILLED;
        }
    }
    else if (spread_capture.status == SpreadCaptureConfig::Status::WAITING_FOR_HEDGE_ORDER_FILLED)
    {
        if (spread_capture.hedge_order.status == Order::Status::FILLED)
        {
            spread_capture.initial_order = nullptr;
            spread_capture.hedge_order = nullptr;
            spread_capture.status = SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER;
        }
        else
        {
            if (spread_capture.initial_order.side == Order::Side::BUY)
            {
                if (mid_price < spread_capture.initial_order.price - spread_capture.stop_loss)
                {
                    spread_capture.status = SpreadCaptureConfig::Status::STOP_LOSS;
                }
            }
            else if (spread_capture.initial_order.side == Order::Side::SELL)
            {
                if (mid_price > spread_capture.initial_order.price + spread_capture.stop_loss)
                {
                    spread_capture.status = SpreadCaptureConfig::Status::STOP_LOSS;
                }
            }
        }
    }
    else if (spread_capture.status == SpreadCaptureConfig::Status::STOP_LOSS)
    {
        if (spread_capture.hedge_order.status != Order::Status::FILLED)
        {
            spread_capture.hedge_order = nullptr;
            spread_capture.initial_order = nullptr;
            spread_capture.status = SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER;
        }
    }
}
