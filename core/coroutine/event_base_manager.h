#pragma once

#include <thread>
#include <vector>
#include <unordered_map>

#include <utils/spin_lock.h>
#include <utils/thread_pinning.h>
#include <enum_reflect/enum_reflect.h>
#include "event_base.h"
#include "epoll_base.h"

enum EpollBaseID
{
    SYSTEM_IO_TASK = 0,       // All of tasks belong to system IO like: timer, socket, saving data to DB, ...
};

enum EventBaseID
{
    ORDER = 1,                // OrderManager
    GATEWAY,                  // Gateway

    MARKET_MAKER_STRATEGY,    // Strategy - Market Maker
    BUY_SPOT_STRATEGY,        // Strategy - Buy Spot
    MEAN_REVERSION_STRATEGY,  // Strategy - Mean Reversion Strategy
    PRICE_ARBITRAGE_STRATEGY, // Strategy - Price Arbitrage
    TREND_FOLLOW_STRATEGY     // Strategy - Trend Follow
};

class EventBaseManager
{
public:
    template <typename T>
    static EventBase* get_event_base_by_id(T id)
    {
        static SpinLock spin_lock;
        static std::vector<std::thread> threads;
        static std::unordered_map<T, std::shared_ptr<EventBase>> event_base_list;

        SpinLockGuard lock(spin_lock);

        if (event_base_list.find(id) == event_base_list.end())
        {
            std::shared_ptr<EventBase> event_base;

            if constexpr (std::is_same_v<T, EpollBaseID>)
            {
                event_base = std::make_shared<EpollBase>(static_cast<EpollBaseID>(id));
            }
            else
            {
                event_base = std::make_shared<EventBase>(static_cast<EventBaseID>(id));
            }

            event_base_list.insert(std::make_pair(id, event_base));
            threads.emplace_back([event_base]()
            {
                // Pin each event base thread to a specific core
                ThreadPinning::pin_thread_to_core(static_cast<int>(event_base->m_event_base_id));
                event_base->loop();
            });
        }

        return event_base_list[id].get();
    }
};