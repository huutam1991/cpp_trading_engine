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

#include "event_base_id.h"
#include "task_info_event.h"

#define MAX_TASK_INFO 200000

class EventBase;
struct BasePromiseType;

class EventBase
{
public:
    EventBase() {}
    EventBase(EventBaseID id) : m_event_base_id {(id)} {}
    virtual ~EventBase() {}

private:
    using TaskEventQueue = MPSCQueue<TaskInfoEvent, MAX_TASK_INFO>;
    TaskEventQueue m_task_event_queue;

public:
    EventBaseID m_event_base_id = EventBaseID::NONE;
    std::atomic<bool> m_stopping{false};

    virtual inline void add_run_task_event(BasePromiseType* promise)
    {
        m_task_event_queue.push(TaskInfoEvent{TaskInfoEvent::TaskType::RUN, promise});
    }

    virtual inline void add_set_suspend_value_event(BasePromiseType* promise)
    {
        m_task_event_queue.push(TaskInfoEvent{TaskInfoEvent::TaskType::SET_SUSPEND_VALUE, promise});
    }

    virtual inline void add_remove_awaiter_event(BasePromiseType* promise);

    virtual inline void add_force_destroy_event(BasePromiseType* promise)
    {
        m_task_event_queue.push(TaskInfoEvent{TaskInfoEvent::TaskType::FORCE_DESTROY, promise});
    }

    virtual void stop();
    virtual void loop();
};