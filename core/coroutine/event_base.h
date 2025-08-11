#pragma once

#include <unordered_map>
#include <queue>
#include <coroutine>
#include <thread>
#include <iostream>

#include <cache/cache_pool.h>
#include <utils/util_macros.h>
#include <utils/spin_lock.h>

#define MAX_TASK_INFO 20000

struct TaskInfo
{
    std::coroutine_handle<> handle = nullptr;
    void* base_promise_type_address = nullptr;

    void clear()
    {
        if (handle != nullptr)
        {
            handle.destroy();
        }
        handle = nullptr;
        base_promise_type_address = nullptr;
    }
};

using TaskInfoPool = CachePool<TaskInfo, MAX_TASK_INFO>;

class EventBase
{
public:
    EventBase() {}
    EventBase(size_t id) : m_event_base_id {id} {}

    size_t m_event_base_id = 0;
    uint64_t m_event_id = 1;
    std::queue<TaskInfo*> m_ready_tasks;

    // Spin lock for fast locking
    SpinLock m_spin_lock;

    void* add_to_event_base(std::coroutine_handle<> handle, void* base_promise_type_address);
    void remove_from_event_base(void* id);
    void set_ready_task(void* task_info);
    TaskInfo* get_ready_task();
    void check_to_remove_task(TaskInfo* task_info);
    void loop();
};