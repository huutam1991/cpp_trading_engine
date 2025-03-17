#ifndef EVENT_BASE_H
#define EVENT_BASE_H

#include <unordered_map>
#include <queue>
#include <coroutine>
#include <thread>
#include <iostream>
#include <mutex>

class EventBase
{
public:
    uint64_t m_event_id = 1;
    std::unordered_map<uint64_t, std::coroutine_handle<>> m_task_list;
    std::queue<uint64_t> m_ready_tasks;

    // Mutex for locking
    std::mutex m_mutex;

    uint64_t get_event_id()
    {
        return m_event_id++;
    }

    uint64_t add_to_event_base(std::coroutine_handle<> handle)
    {
        std::unique_lock lock(m_mutex);

        uint64_t id = get_event_id();
        m_task_list.insert(std::make_pair(id, handle));

        return id;
    }

    void remove_from_event_base(uint64_t id)
    {
        if (m_task_list.find(id) != m_task_list.end())
        {
            std::unique_lock lock(m_mutex);
            m_task_list.erase(id);
        }

        ADD_LOG("Total task list remaining: " << m_task_list.size());
    }

    void set_ready_task(uint64_t task_id)
    {
        std::unique_lock lock(m_mutex);
        m_ready_tasks.push(task_id);
    }

    std::coroutine_handle<> get_ready_task()
    {
        if (m_ready_tasks.empty()) return nullptr;

        std::unique_lock lock(m_mutex);
        uint64_t task_id = m_ready_tasks.front();
        m_ready_tasks.pop();

        return m_task_list[task_id];
    }

    void loop()
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

};

#endif // EVENT_BASE_H