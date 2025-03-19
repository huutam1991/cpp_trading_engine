#ifndef EVENT_BASE_H
#define EVENT_BASE_H

#include <unordered_map>
#include <queue>
#include <coroutine>
#include <thread>
#include <iostream>
#include <mutex>
#include <util_macros.h>

class EventBase
{
public:
    uint64_t m_event_id = 1;
    std::unordered_map<uint64_t, std::coroutine_handle<>> m_task_list;
    std::queue<uint64_t> m_ready_tasks;

    // Mutex for locking
    std::mutex m_mutex;

    uint64_t get_event_id()
    {
        return m_event_id++;
    }

    uint64_t add_to_event_base(std::coroutine_handle<> handle);
    void remove_from_event_base(uint64_t id);
    void set_ready_task(uint64_t task_id);
    std::coroutine_handle<> get_ready_task();
    void loop();
};

#endif // EVENT_BASE_H