#pragma once

#include <vector>

#include <json/json.h>
#include <volume_stat/trade_volume_at_price.h>

#define MAX_VOLUME_STAT_SIZE 200000

class VolumeStat
{
    std::vector<TradeVolumeAtPrice> m_trade_volumes;
    size_t m_max_price_index = 0;
    size_t m_min_price_index = MAX_VOLUME_STAT_SIZE - 1;

    void update_max_min_price_index(size_t price_index);

public:
    VolumeStat();
    VolumeStat(size_t size);

    void add_trade_volume(const TradeUpdate& trade);
    void remove_old_volumes(size_t duration_in_seconds);

    const TradeVolumeAtPrice* get_trade_volume_at_price(double price);

    Json get_data();
};