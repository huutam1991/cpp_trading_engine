#include <coroutine/event_base.h>
#include <coroutine/base_promise_type.h>
#include <spdlog/spdlog.h>

void TaskInfo::check_handle()
{
    if (handle != nullptr && handle.done() == false)
    {
        if (is_first_time == true)
        {
            is_first_time = false;
            auto duration = std::chrono::high_resolution_clock::now() - start;
            auto duration_count = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

            std::string name = event_base->m_event_base_id == 0 ? "EpollBase" : "EventBase";

            // spdlog::debug("{}: {}, Task first wait time: {} microsecond", name, event_base->m_event_base_id, duration_count / 1000.0);
        }

        handle.resume();

        if (handle.done() == true && event_base != nullptr)
        {
            event_base->check_to_remove_task(this);
        }
    }
}

int TaskInfo::generate_fd()
{
    fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    return fd;
}

int TaskInfo::activate()
{
    // Nothing to do for TaskInfo
    return 0;
}

int TaskInfo::handle_read()
{
    check_handle();
    // Always return -1 to indicate this task is done
    return -1;
}

int TaskInfo::handle_write()
{
    // Nothing to do for write event
    return 0;
}

void TaskInfo::release()
{
    // Do nothing here, will be release at EventBase
}

void* EventBase::add_to_event_base(std::coroutine_handle<> handle, void* base_promise_type_address)
{
    TaskInfo* task_info = TaskInfoPool::acquire();
    task_info->handle = handle;
    task_info->base_promise_type_address = base_promise_type_address;
    task_info->event_base = this;
    task_info->start = std::chrono::high_resolution_clock::now();
    task_info->is_first_time = true;

    // spdlog::info("EventBase: {}, Total task list remaining - add: {} ", m_event_base_id, m_ready_task_queue.size());

    return task_info;
}

void EventBase::remove_from_event_base(void* id)
{
    TaskInfoPool::release(static_cast<TaskInfo*>(id));

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
        if (task_info != nullptr)
        {
            task_info->check_handle();
        }
    }
}