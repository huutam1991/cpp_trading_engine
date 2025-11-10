#pragma once

#include <queue>
#include <chrono>
#include <strategy/strategy_abstract.h>
#include <utils/utils.h>

struct TradeVolumeAtPrice
{
    double price = 0.0;
    double total_buy_volume = 0.0;
    double total_sell_volume = 0.0;

    std::queue<TradeUpdate> trades;

    void add_volume(const TradeUpdate& trade)
    {
        price = (size_t)trade.price;

        if (trade.is_buy)
        {
            total_buy_volume += trade.quantity;
        }
        else
        {
            total_sell_volume += trade.quantity;
        }
        trades.push(trade);
    }

    void remove_old_trades(size_t duration_in_seconds)
    {
        if (trades.empty())
        {
            return;
        }

        auto duration = duration_in_seconds * 1000000000; // Convert to nanoseconds
        auto now = Utils::get_time_now_in_utc_nanoseconds();

        while (trades.empty() == false)
        {
            auto& trade = trades.front();
            auto time_diff = now - trade.timestamp;
            if (time_diff > duration)
            {
                // spdlog::debug("Remove old trade at price: {}, time_diff: {}, duration: {}, now: {}, trade_time: {}",
                //     trade.price, time_diff, duration, now, trade.timestamp);
                // Remove old trade
                if (trade.is_buy)
                {
                    total_buy_volume -= trade.quantity;
                }
                else
                {
                    total_sell_volume -= trade.quantity;
                }
                trades.pop();
            }
            else
            {
                break;
            }
        }
    }
};