#pragma once

#include <thread>
#include <vector>
#include <unordered_map>

#include <utils/util_macros.h>
#include <utils/spin_lock.h>
#include <utils/thread_pinning.h>
#include <enum_reflect/enum_reflect.h>
#include "event_base.h"
#include "epoll_base.h"

class EventBaseManager
{
public:
    static EventBase* get_event_base_by_id(int id)
    {
        static SpinLock spin_lock;
        static std::vector<std::thread> threads;
        static std::unordered_map<int, std::shared_ptr<EventBase>> event_base_list;

        SpinLockGuard lock(spin_lock);

        if (event_base_list.find(id) == event_base_list.end())
        {
            std::shared_ptr<EventBase> event_base = std::make_shared<EventBase>(id);

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