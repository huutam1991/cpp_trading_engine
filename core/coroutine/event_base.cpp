#include <coroutine/event_base.h>
#include <coroutine/base_promise_type.h>
#include <spdlog/spdlog.h>

void EventBase::stop()
{
    m_stopping.store(true, std::memory_order_release);
}

void EventBase::add_remove_awaiter_event(BasePromiseType* promise)
{
    m_task_event_queue.push(TaskInfoEvent{TaskInfoEvent::TaskType::REMOVE_AWAITER, promise});
}

void EventBase::loop()
{
    while (m_stopping.load(std::memory_order_acquire) == false)
    {
        // Check if there's any task ready to process
        TaskInfoEvent task_event = m_task_event_queue.pop();

        // Continue process this task
        if (task_event != nullptr)
        {
            task_event.check_handle();
            continue;
        }
        else
        {
            _mm_pause();
        }
    }
}