#include <volume_stat/volume_stat.h>

VolumeStat::VolumeStat()
{
    m_trade_volumes.reserve(MAX_VOLUME_STAT_SIZE);
    m_max_price_index = 0;
    m_min_price_index = m_trade_volumes.size() - 1;
}

VolumeStat::VolumeStat(size_t size)
{
    m_trade_volumes.reserve(size);
    m_max_price_index = 0;
    m_min_price_index = m_trade_volumes.size() - 1;
}

void VolumeStat::update_max_min_price_index(size_t price_index)
{
    m_max_price_index = std::max(m_max_price_index, price_index);
    m_min_price_index = std::min(m_min_price_index, price_index);
}

void VolumeStat::add_trade_volume(const TradeUpdate& trade)
{
    size_t price_index = (size_t)trade.price;
    if (price_index >= m_trade_volumes.size())
    {
        m_trade_volumes.resize(price_index + 1);
    }

    m_trade_volumes[price_index].add_volume(trade);
    update_max_min_price_index(price_index);
}

void VolumeStat::remove_old_volumes(size_t duration_in_seconds)
{
    for (auto& trade_volume_at_price : m_trade_volumes)
    {
        trade_volume_at_price.remove_old_trades(duration_in_seconds);
    }
}

Json VolumeStat::get_data()
{
    Json data;

    for (size_t i = m_min_price_index; i <= m_max_price_index; i++)
    {
        auto& trade_volume_at_price = m_trade_volumes[i];

        data.push_back({
            {"price", trade_volume_at_price.price},
            {"buy", trade_volume_at_price.total_buy_volume},
            {"sell", trade_volume_at_price.total_sell_volume},
        });
    }

    return data;
}