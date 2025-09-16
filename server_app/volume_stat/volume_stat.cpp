#include <volume_stat/volume_stat.h>

VolumeStat::VolumeStat()
{
    m_trade_volumes.reserve(MAX_VOLUME_STAT_SIZE);
}

VolumeStat::VolumeStat(size_t size)
{
    m_trade_volumes.reserve(size);
}

void VolumeStat::add_trade_volume(double price, double buy_volume, bool is_buy)
{
    size_t price_index = (size_t)price;
    if (price_index >= m_trade_volumes.size())
    {
        m_trade_volumes.resize(price_index + 1);
    }

    m_trade_volumes[price_index].add_volume(buy_volume, is_buy);
}

void VolumeStat::remove_old_volumes(std::chrono::seconds duration)
{
    auto now = std::chrono::steady_clock::now();

    for (auto& trade_volume_at_price : m_trade_volumes)
    {
        trade_volume_at_price.remove_old_volumes(duration);
    }
}