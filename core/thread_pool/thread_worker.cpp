#include <utility>
#include <functional>
#include <iostream>

#include <thread_pool/thread_worker.h>

ThreadWorker::ThreadWorker(ThreadPool* thread_pool) : m_thread_pool(thread_pool)
{
}

void ThreadWorker::operator()()
{
    std::function<void()> func = nullptr;

    while (m_thread_pool->m_is_shut_down == false)
    {
        {
            std::unique_lock lock(m_thread_pool->m_mutex);
            if (m_thread_pool->is_function_queue_empty() && m_thread_pool->m_is_shut_down == false)
            {
                m_thread_pool->m_condition_variable.wait(lock);
            }
            func = m_thread_pool->get_function();
        }

        if (func != nullptr)
        {
            m_thread_pool->update_counting_running_thread(1);
            func();
            func = nullptr;
            m_thread_pool->update_counting_running_thread(-1);
        }
    }
}
