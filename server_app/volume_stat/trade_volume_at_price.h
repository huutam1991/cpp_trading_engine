#pragma once

#include <queue>
#include <chrono>
#include <strategy/strategy_abstract.h>

struct TradeVolumeAtPrice
{
    double price = 0.0;
    double total_buy_volume = 0.0;
    double total_sell_volume = 0.0;

    std::queue<TradeUpdate> trades;

    void add_volume(const TradeUpdate& trade)
    {
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

    void remove_old_trades(size_t duration)
    {
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();

        while (trades.empty() == false)
        {
            auto& trade = trades.front();
            auto time_diff = now - trade.timestamp;
            if (time_diff > duration)
            {
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