#include <coroutine/event_base.h>
#include <coroutine/base_promise_type.h>
#include <spdlog/spdlog.h>

void* EventBase::add_to_event_base(std::coroutine_handle<> handle, void* base_promise_type_address)
{
    auto a = std::chrono::high_resolution_clock::now();
    TaskInfo* task_info = TaskInfoPool::acquire();
    // TaskInfo* task_info = new TaskInfo();
    task_info->handle = handle;
    task_info->base_promise_type_address = base_promise_type_address;
    task_info->start = a;
    task_info->is_first_time = true;

    // spdlog::info("EventBase: {}, Total task list remaining - add: {} ", m_event_base_id, m_task_list.size());

    return task_info;
}

void EventBase::remove_from_event_base(void* id)
{
    TaskInfoPool::release(static_cast<TaskInfo*>(id));
    // TaskInfo* task_info = static_cast<TaskInfo*>(id);
    // delete task_info;

    // spdlog::info("EventBase: {}, Total task list remaining: {} ", m_event_base_id, m_ready_task_queue.size());
}

void EventBase::set_ready_task(void* task_info)
{
    m_ready_task_queue.push(static_cast<TaskInfo*>(task_info));
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
        TaskInfo* task_info = m_ready_task_queue.pop();

        // Continue process this task
        if (task_info != nullptr && task_info->handle != nullptr && task_info->handle.done() == false)
        {
            if (task_info->is_first_time == true)
            {
                task_info->is_first_time = false;
                auto duration = std::chrono::high_resolution_clock::now() - task_info->start;
                auto duration_count = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

                // spdlog::debug("EventBase: {}, Task first wait time: {} microsecond", m_event_base_id, duration_count / 1000.0);
            }

            task_info->handle.resume();

            if (task_info->handle.done() == true)
            {
                check_to_remove_task(task_info);
            }
        }
    }
}