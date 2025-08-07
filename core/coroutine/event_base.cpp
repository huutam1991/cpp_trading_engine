#include <coroutine/event_base.h>
#include <coroutine/base_promise_type.h>
#include <spdlog/spdlog.h>

void* EventBase::add_to_event_base(std::coroutine_handle<> handle, void* base_promise_type_address)
{
    TaskInfo* task_info = TaskInfoPool::acquire();
    task_info->handle = handle;
    task_info->base_promise_type_address = base_promise_type_address;

    // spdlog::info("EventBase: {}, Total task list remaining - add: {} ", m_event_base_id, m_task_list.size());

    return task_info;
}

void EventBase::remove_from_event_base(void* id)
{
    TaskInfoPool::release(static_cast<TaskInfo*>(id));

    // spdlog::info("EventBase: {}, Total task list remaining: {} ", m_event_base_id, TaskInfoPool::total_released_items());
}

void EventBase::set_ready_task(void* task_info)
{
    SpinLockGuard spin_lock_guard(m_spin_lock);
    m_ready_tasks.push(static_cast<TaskInfo*>(task_info));
}

TaskInfo* EventBase::get_ready_task()
{
    SpinLockGuard spin_lock_guard(m_spin_lock);

    if (m_ready_tasks.empty()) return nullptr;

    TaskInfo* task_info = m_ready_tasks.front();
    m_ready_tasks.pop();

    return task_info;
}

void EventBase::check_to_remove_task(TaskInfo* task_info)
{
    // Check if this task is already release, then destroy it's coroutine frame and remove from queue
    BasePromiseType* base_promise = static_cast<BasePromiseType*>(task_info->base_promise_type_address);
    if (base_promise->is_task_release == true)
    {
        remove_from_event_base(task_info);
    }
}

void EventBase::loop()
{
    while (true)
    {
        // Check if there's any task ready to process
        TaskInfo* task_info = get_ready_task();

        // Continue process this task
        if (task_info != nullptr && task_info->handle != nullptr && task_info->handle.done() == false)
        {
            task_info->handle.resume();

            if (task_info->handle.done() == true)
            {
                check_to_remove_task(task_info);
            }
        }
        else
        {
            std::this_thread::yield();
        }
    }
}