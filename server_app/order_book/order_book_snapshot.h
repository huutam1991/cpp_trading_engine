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

class OrderBookSnapShot
{
    const Instrument* instrument = nullptr;

public:
    // Bid, Ask
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;

    OrderBookSnapShot()
    {
        asks.reserve(100);
        bids.reserve(100);
    }

    inline void update_instrument(const Instrument* instr);
    inline void add_bid(double price, double quantity);
    inline void add_ask(double price, double quantity);
    inline void clear();

    inline double get_max_bid();
    inline double get_max_ask();
};

using OrderBookSnapShotPool = CachePool<OrderBookSnapShot, 1000>;