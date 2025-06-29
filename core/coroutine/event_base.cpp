#include <coroutine/event_base.h>
#include <coroutine/base_promise_type.h>
#include <spdlog/spdlog.h>
#include <time/measure_time.h>

uint64_t EventBase::add_to_event_base(std::coroutine_handle<> handle, void* base_promise_type_address)
{
    MeasureTime measure_time("EventBase::add_to_event_base");
    std::unique_lock lock(m_mutex);

    uint64_t id = get_event_id();
    m_task_list.emplace(id, TaskInfo {handle, base_promise_type_address});

    // spdlog::info("EventBase: {}, Total task list remaining - add: {} ", m_event_base_id, m_task_list.size());

    return id;
}

void EventBase::remove_from_event_base(uint64_t id)
{
    MeasureTime measure_time("EventBase::remove_from_event_base");
    SpinLockGuard spin_lock_guard(m_spin_lock);

    if (m_task_list.find(id) != m_task_list.end())
    {
        m_task_list.erase(id);
    }

    // spdlog::info("EventBase: {}, Total task list remaining: {} ", m_event_base_id, m_task_list.size());
}


void EventBase::set_ready_task(uint64_t task_id)
{
    SpinLockGuard spin_lock_guard(m_spin_lock);
    m_ready_tasks.push(task_id);
}

EventBase::TaskInfo EventBase::get_ready_task()
{
    SpinLockGuard spin_lock_guard(m_spin_lock);

    if (m_ready_tasks.empty()) return TaskInfo{};

    uint64_t task_id = m_ready_tasks.front();
    m_ready_tasks.pop();

    return m_task_list[task_id];
}

void EventBase::check_to_remove_task(TaskInfo task_info)
{
    // Check if this task is already release, then destroy it's coroutine frame and remove from queue
    BasePromiseType* base_promise = static_cast<BasePromiseType*>(task_info.base_promise_type_address);
    if (base_promise->is_task_release == true)
    {
        remove_from_event_base(base_promise->task_id);
        task_info.handle.destroy();
    }
}

void EventBase::loop()
{
    while (true)
    {
        // Check if there's any task ready to process
        TaskInfo task_info = get_ready_task();

        // Continue process this task
        if (task_info.handle != nullptr && task_info.handle.done() == false)
        {
            task_info.handle.resume();

            if (task_info.handle.done() == true)
            {
                check_to_remove_task(task_info);
            }
        }
    }
}