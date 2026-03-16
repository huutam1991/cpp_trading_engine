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
                {"total", spread_capture.profit + spread_capture.loss}
            }},
            {
                "current_status", {
                    {"mean_price", spread_capture.mean_price},
                    {"volatility", spread_capture.volatility},
                    {"current_move_distance", spread_capture.current_move_distance},
                    {"current_entry_distance", spread_capture.current_entry_distance},
                    {"current_take_profit", spread_capture.current_take_profit},
                    {"current_stop_loss", spread_capture.current_stop_loss},
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

    return data;
}

void SpreadCaptureConfigManager::handle_order_book_snapshot(OrderBookSnapShot* snapshot)
{
    // MeasureTime t("StrategyMeanReversionStateStop - handle_order_book_snapshot", MeasureUnit::MICROSECOND);
    double mid_price = snapshot->get_mid_price();
    volatility_estimator.update(mid_price);

    for (auto& spread_capture : spread_captures)
    {
        double volatility = volatility_estimator.stddev();
        spread_capture.mean_price = volatility_estimator.mean();
        spread_capture.volatility = volatility;

        if (spread_capture.status == SpreadCaptureConfig::Status::NONE)
        {
            spread_capture.current_move_distance = volatility * spread_capture.move_distance;
            spread_capture.current_entry_distance = volatility * spread_capture.entry_distance;
            spread_capture.current_take_profit = volatility * spread_capture.take_profit;
            spread_capture.current_stop_loss = volatility * spread_capture.stop_loss;

            if (std::abs(mid_price - spread_capture.mean_price) < spread_capture.current_move_distance)
            {
                continue; // Price has not moved enough, do nothing
            }

            if (mid_price > spread_capture.mean_price)
            {
                // Price moved up, we want to sell high and buy back low
                spread_capture.sell_order.status = Order::Status::NEW;
                spread_capture.sell_order.price = spread_capture.mean_price + spread_capture.current_entry_distance;
                spread_capture.status = SpreadCaptureConfig::Status::PLACING_INIT_SELL_ORDER;
            }
            else
            {
                // Price moved down, we want to buy low and sell back high
                spread_capture.buy_order.status = Order::Status::NEW;
                spread_capture.buy_order.price = spread_capture.mean_price - spread_capture.current_entry_distance;
                spread_capture.status = SpreadCaptureConfig::Status::PLACING_INIT_BUY_ORDER;
            }
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_INIT_BUY_ORDER)
        {
            if (mid_price < spread_capture.buy_order.price)
            {
                spread_capture.buy_order.status = Order::Status::FILLED;
                spread_capture.status = SpreadCaptureConfig::Status::PLACING_HEDGE_SELL_ORDER;
                spread_capture.sell_order.status = Order::Status::NEW;
                spread_capture.sell_order.price = spread_capture.buy_order.price + spread_capture.current_take_profit;
            }
            else if (mid_price >= spread_capture.buy_order.price + spread_capture.current_entry_distance + (spread_capture.current_move_distance) / 3)
            {
                // Price go back to entry price, cancel buy order and reset
                spread_capture.buy_order.status = Order::Status::CANCELED;
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
            }
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_INIT_SELL_ORDER)
        {
            if (mid_price > spread_capture.sell_order.price)
            {
                spread_capture.sell_order.status = Order::Status::FILLED;
                spread_capture.status = SpreadCaptureConfig::Status::PLACING_HEDGE_BUY_ORDER;
                spread_capture.buy_order.status = Order::Status::NEW;
                spread_capture.buy_order.price = spread_capture.sell_order.price - spread_capture.current_take_profit;
            }
            else if (mid_price <= spread_capture.sell_order.price - spread_capture.current_entry_distance - (spread_capture.current_move_distance) / 3)
            {
                // Price go back to entry price, cancel sell order and reset
                spread_capture.sell_order.status = Order::Status::CANCELED;
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
            }
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_BUY_ORDER)
        {
            if (mid_price < spread_capture.buy_order.price)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.success++;
                spread_capture.profit += spread_capture.current_take_profit;

                spread_capture.buy_order = nullptr;
                spread_capture.sell_order = nullptr;
            }
            else if (mid_price >= spread_capture.sell_order.price + spread_capture.current_stop_loss)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.fail++;
                spread_capture.loss -= spread_capture.current_stop_loss;

                spread_capture.buy_order = nullptr;
                spread_capture.sell_order = nullptr;
            }
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_SELL_ORDER)
        {
            if (mid_price > spread_capture.sell_order.price)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.success++;
                spread_capture.profit += spread_capture.current_take_profit;

                spread_capture.buy_order = nullptr;
                spread_capture.sell_order = nullptr;
            }
            else if (mid_price <= spread_capture.buy_order.price - spread_capture.current_stop_loss)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.fail++;
                spread_capture.loss -= spread_capture.current_stop_loss;

                spread_capture.buy_order = nullptr;
                spread_capture.sell_order = nullptr;
            }
        }
    }
}
