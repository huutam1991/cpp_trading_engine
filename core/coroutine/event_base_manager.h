#pragma once

#include <thread>
#include <unordered_map>
#include <memory>

#include <utils/spin_lock.h>
#include <utils/thread_pinning.h>
#include <enum_reflect/enum_reflect.h>
#include "event_base.h"
#include "epoll_base.h"

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
};

class EventBaseManager
{
public:
    template <typename T>
    static EventBase* get_event_base_by_id(T id)
    {
        static SpinLock spin_lock;
        std::unordered_map<T, std::shared_ptr<EventBase>>& event_base_list = get_event_bases<T>();
        std::vector<std::thread>& event_base_threads = get_event_base_threads();

        SpinLockGuard lock(spin_lock);

        auto it = event_base_list.find(id);
        if (it == event_base_list.end())
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

            event_base_list.emplace(id, event_base);

            std::thread thread([event_base]()
            {
                // Pin each event base thread to a specific core
                ThreadPinning::pin_thread_to_core(static_cast<int>(event_base->m_event_base_id));
                event_base->loop();
            });

            event_base_threads.push_back(std::move(thread));

            return event_base.get();
        }

        return it->second.get();
    }

    template <typename T>
    static std::unordered_map<T, std::shared_ptr<EventBase>>& get_event_bases()
    {
        static std::unordered_map<T, std::shared_ptr<EventBase>> event_bases;
        return event_bases;
    }

    static std::vector<std::thread>& get_event_base_threads()
    {
        static std::vector<std::thread> event_base_threads;
        return event_base_threads;
    }

    static void shutdown_all()
    {
        // Signal all event bases to stop
        std::unordered_map<EventBaseID, std::shared_ptr<EventBase>>& event_base_list = get_event_bases<EventBaseID>();
        std::unordered_map<EpollBaseID, std::shared_ptr<EventBase>>& epoll_base_list = get_event_bases<EpollBaseID>();
        {
            for (auto& [id, event_base] : event_base_list)
            {
                event_base->stop();
            }

            for (auto& [id, event_base] : epoll_base_list)
            {
                event_base->stop();
            }
        }

        std::vector<std::thread>& event_base_threads = get_event_base_threads();
        for (auto& thread : event_base_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        event_base_list.clear();
        epoll_base_list.clear();
        event_base_threads.clear();
    }
};