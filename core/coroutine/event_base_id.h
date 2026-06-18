#pragma once

#include <cstddef>

enum EventBaseID
{
    NONE = -1,
    EPOLL_SYSTEM_IO_TASK = 0, // All of tasks belong to system IO like: timer, socket, saving data to DB, ...
    EPOLL_GATEWAY,            // Gateway
    ORDER,                    // OrderManager
    ORDER_BOOK,               // OrderBookManager

    MARKET_MAKER_STRATEGY,    // Strategy - Market Maker
    BUY_SPOT_STRATEGY,        // Strategy - Buy Spot
    MEAN_REVERSION_STRATEGY,  // Strategy - Mean Reversion Strategy
    PRICE_ARBITRAGE_STRATEGY, // Strategy - Price Arbitrage
    TREND_FOLLOW_STRATEGY,    // Strategy - Trend Follow
    NO_STRATEGY,              // Strategy - No Strategy
    TOTAL
};


inline thread_local int CURRENT_EVENT_BASE = EventBaseID::NONE;