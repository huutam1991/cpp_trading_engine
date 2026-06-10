#pragma once

#include <vector>

#include <cache/shared_cache_pool.h>
#include <instrument/instrument.h>

struct OrderBookLevel
{
    double price;
    double quantity;

    OrderBookLevel() : price(0.0), quantity(0.0) {}
    OrderBookLevel(double p, double q) : price(p), quantity(q) {}
};

class OrderBookSnapShot
{
public:
    const Instrument* instrument = nullptr;

    // Bid, Ask
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;
    size_t bids_size = 0;
    size_t asks_size = 0;
    size_t max_levels = 0;

    OrderBookSnapShot()
    {
        asks.resize(200);
        bids.resize(200);

        max_levels = 200;
    }

    void update_instrument(const Instrument* instr)
    {
        instrument = instr;
    }

    void resize(size_t new_size)
    {
        max_levels = new_size;
    }

    void refresh();
    void print_order_book();

    inline void add_bid(double price, double quantity)
    {
        if (bids_size >= max_levels)
        {
            return;
        }

        bids[bids_size].price = price;
        bids[bids_size++].quantity = quantity;
    }

    inline void add_ask(double price, double quantity)
    {
        if (asks_size >= max_levels)
        {
            return;
        }

        asks[asks_size].price = price;
        asks[asks_size++].quantity = quantity;
    }

    double get_mid_price();
    double get_best_bid();
    double get_best_ask();
    double get_best_bid_quantity();
    double get_best_ask_quantity();
    double get_bid_volume();
    double get_ask_volume();
};

using OrderBookSnapShotPool = SharedCachePool<OrderBookSnapShot, 10000>;
using OrderBookSnapShotObject = OrderBookSnapShotPool::ObjectPointer;