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
    struct TaskInfo
    {
        std::coroutine_handle<> handle = nullptr;
        void* base_promise_type_address = nullptr;
    };

public:
    EventBase() {}
    EventBase(size_t id) : m_event_base_id {id} {}

    size_t m_event_base_id = 0;
    uint64_t m_event_id = 1;
    std::unordered_map<uint64_t, TaskInfo> m_task_list;
    std::queue<uint64_t> m_ready_tasks;

    // Mutex for locking
    std::mutex m_mutex;

    uint64_t get_event_id()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

        return static_cast<uint64_t>(nanos);
    }

    uint64_t add_to_event_base(std::coroutine_handle<> handle, void* base_promise_type_address);
    void remove_from_event_base(uint64_t id);
    void set_ready_task(uint64_t task_id);
    TaskInfo get_ready_task();
    void check_to_remove_task(TaskInfo task_info);
    void loop();
};

#endif // EVENT_BASE_H