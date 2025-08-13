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
public:
    const Instrument* instrument = nullptr;

    // Bid, Ask
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;

    OrderBookSnapShot()
    {
        asks.reserve(100);
        bids.reserve(100);
    }

    void update_instrument(const Instrument* instr);
    void clear();
    void print_order_book();

    inline void add_bid(double price, double quantity)
    {
        bids.emplace_back(price, quantity);
    }

    inline void add_ask(double price, double quantity)
    {
        asks.emplace_back(price, quantity);
    }

    double get_best_bid();
    double get_best_ask();
    double get_best_bid_quantity();
    double get_best_ask_quantity();
    double get_bid_volume();
    double get_ask_volume();
};

using OrderBookSnapShotPool = CachePool<OrderBookSnapShot, 1000>;