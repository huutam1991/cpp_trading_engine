#pragma once

#include <queue>
#include <chrono>

struct TradeVolumeByTime
{
    double buy_volume = 0.0;
    double sell_volume = 0.0;
    std::chrono::time_point<std::chrono::steady_clock> time;
};

struct TradeVolumeAtPrice
{
    double price = 0.0;
    double total_buy_volume = 0.0;
    double total_sell_volume = 0.0;

    std::queue<TradeVolumeByTime> volumes;

    void add_volume(double buy_volume, bool is_buy)
    {
        auto now = std::chrono::steady_clock::now();

        TradeVolumeByTime volume_by_time;
        volume_by_time.time = now;
        if (is_buy)
        {
            volume_by_time.buy_volume = buy_volume;
            total_buy_volume += buy_volume;
        }
        else
        {
            volume_by_time.sell_volume = buy_volume;
            total_sell_volume += buy_volume;
        }
        volumes.push(volume_by_time);
    }

    void remove_old_volumes(std::chrono::seconds duration)
    {
        auto now = std::chrono::steady_clock::now();

        while (volumes.empty() == false)
        {
            auto& volume_by_time = volumes.front();
            auto time_diff = now - volume_by_time.time;
            if (time_diff > duration)
            {
                // Remove old volume
                if (volume_by_time.buy_volume > 0)
                {
                    total_buy_volume -= volume_by_time.buy_volume;
                }
                if (volume_by_time.sell_volume > 0)
                {
                    total_sell_volume -= volume_by_time.sell_volume;
                }
                volumes.pop();
            }
            else
            {
                break;
            }
        }
    }
};