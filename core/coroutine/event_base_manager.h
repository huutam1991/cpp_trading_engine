#pragma once

#include <thread>
#include <vector>
#include <unordered_map>

#include <utils/util_macros.h>
#include <utils/spin_lock.h>
#include <enum_reflect/enum_reflect.h>
#include "event_base.h"
#include "epoll_base.h"

template <class EnumType>
class EventBaseManager
{
public:
    static EventBase* get_event_base_by_id(EnumType id)
    {
        static SpinLock spin_lock;
        static std::vector<std::thread> threads;
        static std::unordered_map<EnumType, std::shared_ptr<EventBase>> event_base_list;

        SpinLockGuard lock(spin_lock);

        if (event_base_list.find(id) == event_base_list.end())
        {
            std::shared_ptr<EventBase> event_base;

            if (enum_reflect::enum_name<EnumType>(id).find("SYSTEM_IO") != std::string::npos)
            {
                event_base = std::make_shared<EpollBase>(id);
            }
            else
            {
                event_base = std::make_shared<EventBase>(id);
            }

            // // Hard code for EpollBase with id = 0
            // if (id == 0)
            // {

            // }
            // else
            // {
            // }
            event_base_list.insert(std::make_pair(id, event_base));
            threads.emplace_back([event_base]()
            {
                event_base->loop();
            });
        }

        return event_base_list[id].get();
    }
};