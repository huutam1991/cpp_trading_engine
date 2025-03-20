#include <coroutine/event_base.h>
#include <coroutine/base_promise_type.h>

uint64_t EventBase::add_to_event_base(std::coroutine_handle<> handle)
{
    std::unique_lock lock(m_mutex);

    uint64_t id = get_event_id();
    m_task_list.insert(std::make_pair(id, handle));

    return id;
}

void EventBase::remove_from_event_base(uint64_t id)
{
    if (m_task_list.find(id) != m_task_list.end())
    {
        std::unique_lock lock(m_mutex);
        m_task_list.erase(id);
    }

    ADD_LOG("Total task list remaining: " << m_task_list.size());
}


void EventBase::set_ready_task(uint64_t task_id)
{
    std::unique_lock lock(m_mutex);
    m_ready_tasks.push(task_id);
}

std::coroutine_handle<> EventBase::get_ready_task()
{
    if (m_ready_tasks.empty()) return nullptr;

    std::unique_lock lock(m_mutex);
    uint64_t task_id = m_ready_tasks.front();
    m_ready_tasks.pop();

    return m_task_list[task_id];
}

void EventBase::check_to_remove_task(std::coroutine_handle<> handle)
{
    // Check if this task is already release, then destroy it's coroutine frame and remove from queue
    ADD_LOG("check crash " << 4);
    BasePromiseType* base_promise = static_cast<BasePromiseType*>(handle.address());
    ADD_LOG("check crash " << 5);
    if (base_promise->is_task_release == true)
    {
        ADD_LOG("check crash " << 6);
        remove_from_event_base(base_promise->task_id);
        ADD_LOG("check crash " << 7);
        handle.destroy();
        ADD_LOG("check crash " << 8);
    }
}

void EventBase::loop()
{
    while (true)
    {
        // Check if there's any task ready to process
        std::coroutine_handle<> handle = get_ready_task();

        // Continue process this task
        if (handle != nullptr && handle.done() == false)
        {
            ADD_LOG("check crash " << 1);
            handle.resume();
            ADD_LOG("check crash " << 2);

            if (handle.done() == true)
            {
                ADD_LOG("check crash " << 3);
                check_to_remove_task(handle);
            }
        }
    }
}