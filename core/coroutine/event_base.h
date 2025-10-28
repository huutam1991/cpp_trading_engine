#pragma once

#include <unordered_map>
#include <queue>
#include <coroutine>
#include <thread>
#include <iostream>

#include <cache/cache_pool.h>
#include <utils/util_macros.h>
#include <queue/mpsc_queue.h>
#include <utils/spin_lock.h>

#define MAX_TASK_INFO 20000

class EventBase;

struct TaskInfo
{
    std::coroutine_handle<> handle = nullptr;
    void* base_promise_type_address = nullptr;
    EventBase* event_base = nullptr;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    bool is_first_time = true;

    void clear()
    {
        if (handle != nullptr)
        {
            handle.destroy();
        }
        handle = nullptr;
        base_promise_type_address = nullptr;
        event_base = nullptr;
    }

    void check_handle();
};

using TaskInfoPool = CachePool<TaskInfo, MAX_TASK_INFO>;
using ReadyTaskQueue = MPSCQueue<TaskInfo, MAX_TASK_INFO>;

class EventBase
{
public:
    EventBase() {}
    EventBase(size_t id) : m_event_base_id {id} {}

    size_t m_event_base_id = 0;
    ReadyTaskQueue m_ready_task_queue;

    void* add_to_event_base(std::coroutine_handle<> handle, void* base_promise_type_address);
    void remove_from_event_base(void* id);
    void check_to_remove_task(TaskInfo* task_info);
    virtual void set_ready_task(void* task_info);
    virtual void loop();
};