#pragma once

#include <unordered_map>
#include <queue>
#include <coroutine>
#include <thread>
#include <iostream>
#include <sys/eventfd.h>

#include <cache/cache_pool.h>
#include <queue/mpsc_queue.h>
#include <system_io/system_io_object.h>

#define MAX_TASK_INFO 200000

class EventBase;

struct TaskInfo : public NamedIOObject<TaskInfo>
{
    std::coroutine_handle<> handle = nullptr;
    void* base_promise_type_address = nullptr;
    EventBase* event_base = nullptr;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    bool is_first_time = true;

    void refresh()
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

    // SystemIOObject's methods
    virtual int generate_fd() override;
    virtual int get_io_events() { return EPOLLIN; }
    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;
};

using TaskInfoPool = CachePool<TaskInfo, MAX_TASK_INFO>;
using ReadyTaskQueue = MPSCQueue<TaskInfo*, MAX_TASK_INFO>;

class EventBase
{
public:
    EventBase() {}
    EventBase(size_t id) : m_event_base_id {id} {}
    virtual ~EventBase() {}

private:
    enum TaskType
    {
        NONE,
        RUN,
        SET_SUSPEND_VALUE,
        REMOVE_AWAITER
    };

    struct TaskInfoEvent
    {
        TaskType type;
        std::coroutine_handle<> handle;

        TaskInfoEvent() = default;
        TaskInfoEvent(std::nullptr_t) : type(TaskType::NONE), handle(nullptr) {}
        TaskInfoEvent(TaskType type, std::coroutine_handle<> handle) : type(type), handle(handle) {}

        bool operator==(std::nullptr_t) const
        {
            return type == TaskType::NONE && handle == nullptr;
        }
    };

    using TaskEventQueue = MPSCQueue<TaskInfoEvent, MAX_TASK_INFO>;
    TaskEventQueue m_task_event_queue;

public:

    size_t m_event_base_id = 0;
    std::atomic<bool> m_stopping{false};
    ReadyTaskQueue m_ready_task_queue;

    void* create_task_info(std::coroutine_handle<> handle, void* base_promise_type_address);
    void remove_from_event_base(void* id);
    void check_to_remove_task(TaskInfo* task_info);

    inline void add_run_task_event(std::coroutine_handle<> handle)
    {
        m_task_event_queue.push(TaskInfoEvent{TaskType::RUN, handle});
    }

    inline void add_set_suspend_value_event(std::coroutine_handle<> handle)
    {
        m_task_event_queue.push(TaskInfoEvent{TaskType::SET_SUSPEND_VALUE, handle});
    }

    inline void add_remove_awaiter_event(std::coroutine_handle<> handle)
    {
        m_task_event_queue.push(TaskInfoEvent{TaskType::REMOVE_AWAITER, handle});
    }

    virtual void stop();
    virtual void set_ready_task(void* task_info);
    virtual void loop();
    virtual void loop2();
};