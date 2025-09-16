#include <volume_stat/volume_stat.h>

VolumeStat::VolumeStat()
{
    m_trade_volumes.reserve(MAX_VOLUME_STAT_SIZE);
}

VolumeStat::VolumeStat(size_t size)
{
    m_trade_volumes.reserve(size);
}

void VolumeStat::add_trade_volume(const TradeUpdate& trade)
{
    size_t price_index = (size_t)trade.price;
    if (price_index >= m_trade_volumes.size())
    {
        m_trade_volumes.resize(price_index + 1);
    }

    m_trade_volumes[price_index].add_volume(trade);
}

void VolumeStat::remove_old_volumes(size_t duration_in_seconds)
{
    for (auto& trade_volume_at_price : m_trade_volumes)
    {
        trade_volume_at_price.remove_old_trades(duration_in_seconds);
    }
}