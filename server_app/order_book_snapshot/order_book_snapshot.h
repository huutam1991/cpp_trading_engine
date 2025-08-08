#pragma once

#include <vector>

#include <cache/cache_pool.h>
#include <instrument/instrument.h>

struct OrderBookLevel
{
    double price;
    double quantity;

    OrderBookLevel(double p, double q) : price(p), quantity(q) {}
};

class OrderbookSnapShot
{
    // Bid, Ask
    std::vector<OrderBookLevel> m_bids;
    std::vector<OrderBookLevel> m_asks;

public:
    OrderbookSnapShot()
    {
        m_asks.reserve(100);
        m_bids.reserve(100);
    }

    double get_max_bid();
    double get_max_ask();
};

using OrderbookSnapshotPool = CachePool<OrderbookSnapShot, 1000>;