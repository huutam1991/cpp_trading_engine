#pragma once

#include <thread>
#include <unordered_map>
#include <memory>

#include <utils/spin_lock.h>
#include <utils/thread_pinning.h>
#include <enum_reflect/enum_reflect.h>
#include "event_base_id.h"
#include "event_base.h"
#include "epoll_base.h"

// TODO: Remove EventBaseID, use prefix EPOLL_ to detect EpollBase when creating event base in EventBaseManager

class EventBaseManager
{
public:
    static EventBase* get_event_base_by_id(EventBaseID id)
    {
        static SpinLock spin_lock;
        std::unordered_map<EventBaseID, std::shared_ptr<EventBase>>& event_base_list = get_event_bases();
        std::vector<std::thread>& event_base_threads = get_event_base_threads();

        SpinLockGuard lock(spin_lock);

        auto it = event_base_list.find(id);
        if (it == event_base_list.end())
        {
            std::shared_ptr<EventBase> event_base;

            std::string_view id_str = enum_reflect::enum_name(id);
            if (id_str.starts_with("EPOLL_"))
            {
                event_base = std::make_shared<EpollBase>(static_cast<EventBaseID>(id));
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

                // Set the thread-local variable CURRENT_EVENT_BASE to the event base's ID
                CURRENT_EVENT_BASE = event_base->m_event_base_id;

                event_base->loop();
            });

            event_base_threads.push_back(std::move(thread));

            return event_base.get();
        }

        return it->second.get();
    }

    static std::unordered_map<EventBaseID, std::shared_ptr<EventBase>>& get_event_bases()
    {
        static std::unordered_map<EventBaseID, std::shared_ptr<EventBase>> event_bases;
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
        std::unordered_map<EventBaseID, std::shared_ptr<EventBase>>& event_base_list = get_event_bases();
        {
            for (auto& [id, event_base] : event_base_list)
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
        event_base_threads.clear();
    }
};