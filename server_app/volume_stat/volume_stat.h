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

    void add_trade_volume(const TradeUpdate& trade);
    void remove_old_volumes(size_t duration_in_seconds);
};