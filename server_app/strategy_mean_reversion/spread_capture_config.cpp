#include "spread_capture_config.h"

void SpreadCaptureConfigManager::reset()
{
    for (auto& spread_capture : spread_captures)
    {
        spread_capture.reset();
    }
}

Json SpreadCaptureConfigManager::get_info()
{
    Json data;

    for (auto& spread_capture : spread_captures)
    {
        data.push_back({
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

    for (auto& spread_capture : spread_captures)
    {
        if (spread_capture.status == SpreadCaptureConfig::Status::NONE)
        {
            spread_capture.buy_order.status = Order::Status::NEW;
            spread_capture.buy_order.price = mid_price - spread_capture.entry_distance;
            spread_capture.sell_order.status = Order::Status::NEW;
            spread_capture.sell_order.price = mid_price + spread_capture.entry_distance;

            spread_capture.status = SpreadCaptureConfig::Status::PLACING_INIT_ORDERS;
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_INIT_ORDERS)
        {
            if (mid_price < spread_capture.buy_order.price)
            {
                spread_capture.buy_order.status = Order::Status::FILLED;
                spread_capture.status = SpreadCaptureConfig::Status::PLACING_HEDGE_SELL_ORDER;
                spread_capture.sell_order.price = spread_capture.buy_order.price + spread_capture.take_profit;
            }
            else if (mid_price > spread_capture.sell_order.price)
            {
                spread_capture.sell_order.status = Order::Status::FILLED;
                spread_capture.status = SpreadCaptureConfig::Status::PLACING_HEDGE_BUY_ORDER;
                spread_capture.buy_order.price = spread_capture.sell_order.price - spread_capture.take_profit;
            }
            else
            {
                // Update new order price
                spread_capture.buy_order.price = mid_price - spread_capture.entry_distance;
                spread_capture.sell_order.price = mid_price + spread_capture.entry_distance;
            }
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_BUY_ORDER)
        {
            if (mid_price < spread_capture.buy_order.price)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.success++;
                spread_capture.profit += spread_capture.take_profit;
            }
            else if (mid_price >= spread_capture.sell_order.price + spread_capture.stop_loss)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.fail++;
                spread_capture.loss -= spread_capture.stop_loss;
            }
        }
        else if (spread_capture.status == SpreadCaptureConfig::Status::PLACING_HEDGE_SELL_ORDER)
        {
            if (mid_price > spread_capture.sell_order.price)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.success++;
                spread_capture.profit += spread_capture.take_profit;
            }
            else if (mid_price <= spread_capture.buy_order.price - spread_capture.stop_loss)
            {
                spread_capture.status = SpreadCaptureConfig::Status::NONE;
                spread_capture.fail++;
                spread_capture.loss -= spread_capture.stop_loss;
            }
        }
    }
}
