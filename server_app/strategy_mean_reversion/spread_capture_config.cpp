#include "spread_capture_config.h"

void SpreadCaptureConfigManager::init_from_config(const std::vector<SpreadCaptureConfig>& configs)
{
    spread_captures = configs;
}

void SpreadCaptureConfigManager::reset()
{
    for (auto& spread_capture : spread_captures)
    {
        spread_capture.reset();
    }
    volatility_estimator.reset();
}

Json SpreadCaptureConfigManager::get_info()
{
    Json data;

    for (auto& spread_capture : spread_captures)
    {
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
                            {"price", spread_capture.buy_order.price},
                            {"status", enum_reflect::enum_name(spread_capture.buy_order.status)}
                        }},
                        {"sell_order", {
                            {"price", spread_capture.sell_order.price},
                            {"status", enum_reflect::enum_name(spread_capture.sell_order.status)}
                        }}
                    }},
                }
            }
        });
    }

    return data;
}

void SpreadCaptureConfigManager::handle_order_update(Order& order)
{
    for (auto& spread_capture : spread_captures)
    {
        if (order.status == Order::Status::NEW)
        {
            if (order.side == Order::Side::BUY)
            {
                spread_capture.buy_order = order;
            }
            else if (order.side == Order::Side::SELL)
            {
                spread_capture.sell_order = order;
            }
        }
        else if (order.status == Order::Status::REJECTED)
        {
            double current_price = volatility_estimator.get_current_price();

            if (order.side == Order::Side::BUY)
            {
                spread_capture.buy_order.price = order.price - std::max(spread_capture.current_take_profit * 0.8, 3.0);
                spread_capture.buy_order.status = Order::Status::NOT_AVAILABLE;
            }
            else if (order.side == Order::Side::SELL)
            {
                spread_capture.sell_order.price = order.price + std::max(spread_capture.current_take_profit * 0.8, 3.0);
                spread_capture.sell_order.status = Order::Status::NOT_AVAILABLE;
            }
        }

        if (spread_capture.buy_order.status == Order::Status::NEW &&
            spread_capture.sell_order.status == Order::Status::NEW)
        {
            spread_capture.status = SpreadCaptureConfig::Status::NONE;
        }
    }
}

void SpreadCaptureConfigManager::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMeanReversionStateStop - handle_order_book_snapshot", MeasureUnit::MICROSECOND);
    double mid_price = snapshot->get_mid_price();
    volatility_estimator.update(mid_price);

    for (auto& spread_capture : spread_captures)
    {
        double volatility = volatility_estimator.stddev();
        spread_capture.volatility = volatility;
        spread_capture.mean_price = volatility_estimator.mean();

        if (spread_capture.mean_price == 0.0)
        {
            continue; // Not enough data to calculate mean price, do nothing
        }

        double prev_price = volatility_estimator.get_prev_price();

        if (spread_capture.status == SpreadCaptureConfig::Status::NONE)
        {
            spread_capture.current_move_distance = volatility * spread_capture.move_distance;
            spread_capture.current_entry_distance = volatility * spread_capture.entry_distance;
            spread_capture.current_take_profit = volatility * spread_capture.take_profit;
            spread_capture.current_stop_loss = volatility * spread_capture.stop_loss;

            if (std::abs(mid_price - prev_price) < spread_capture.current_move_distance)
            {
                continue; // Price has not moved enough, do nothing
            }

            if (mid_price > spread_capture.mean_price)
            {
                // Try to buy first then sell back, we want to sell high and buy back low
                spread_capture.buy_order.status = Order::Status::NOT_AVAILABLE;
                spread_capture.buy_order.price = mid_price - 0.5;
            }
            else
            {
                // Try to sell first then buy back, we want to buy low and sell back high
                spread_capture.sell_order.status = Order::Status::NOT_AVAILABLE;
                spread_capture.sell_order.price = mid_price + 0.5;
            }

            spread_capture.status = SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER;
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_INITIAL_ORDER)
        {
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_ORDER)
        {
        }

    }
}
