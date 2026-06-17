#pragma once

#include <cstddef>

enum EpollBaseID
{
    SYSTEM_IO_TASK = 0,       // All of tasks belong to system IO like: timer, socket, saving data to DB, ...
    GATEWAY,                  // Gateway
};

enum EventBaseID
{
    ORDER = 2,                // OrderManager
    ORDER_BOOK,               // OrderBookManager

    MARKET_MAKER_STRATEGY,    // Strategy - Market Maker
    BUY_SPOT_STRATEGY,        // Strategy - Buy Spot
    MEAN_REVERSION_STRATEGY,  // Strategy - Mean Reversion Strategy
    PRICE_ARBITRAGE_STRATEGY, // Strategy - Price Arbitrage
    TREND_FOLLOW_STRATEGY,    // Strategy - Trend Follow
    NO_STRATEGY,              // Strategy - No Strategy
    TOTAL
};


inline thread_local size_t CURRENT_EVENT_BASE = EventBaseID::TOTAL;