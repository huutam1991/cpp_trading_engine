#pragma once

#include <thread>
#include <vector>
#include <unordered_map>

#include <utils/util_macros.h>
#include <utils/spin_lock.h>
#include "event_base.h"

class EventBaseManager
{
public:
    static EventBase* get_event_base_by_id(size_t id)
    {
        static SpinLock spin_lock;
        static std::vector<std::thread> threads;
        static std::unordered_map<size_t, std::shared_ptr<EventBase>> event_base_list;

        SpinLockGuard lock(spin_lock);

        if (event_base_list.find(id) == event_base_list.end())
        {
            auto event_base = std::make_shared<EventBase>(id);
            event_base_list.insert(std::make_pair(id, event_base));
            threads.emplace_back([event_base]()
            {
                event_base->loop();
            });
        }

        return event_base_list[id].get();
    }
};