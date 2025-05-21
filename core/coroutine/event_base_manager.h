#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>

#include <util_macros.h>
#include "event_base.h"

class EventBaseManager
{
public:
    static EventBase* get_event_base_by_id(size_t id)
    {
        static std::mutex mutex;
        static std::vector<std::thread> threads;
        static std::unordered_map<size_t, std::shared_ptr<EventBase>> event_base_list;

        if (event_base_list.find(id) == event_base_list.end())
        {
            std::unique_lock lock(mutex);

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