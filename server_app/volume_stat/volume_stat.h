#pragma once

#include <vector>

#include <volume_stat/trade_volume_at_price.h>

#define MAX_VOLUME_STAT_SIZE 200000

class VolumeStat
{
    std::vector<TradeVolumeAtPrice> m_trade_volumes;

public:
    VolumeStat();
    VolumeStat(size_t size);

    void add_trade_volume(double price, double buy_volume, bool is_buy);
    void remove_old_volumes(std::chrono::seconds duration);
};