#include <coroutine/event_base.h>

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

void EventBase::loop()
{
    while (true)
    {
        // Check if there's any task ready to process
        std::coroutine_handle<> handle = get_ready_task();

        // Continue process this task
        if (handle != nullptr && handle.done() == false)
        {
            handle.resume();
        }
    }
}