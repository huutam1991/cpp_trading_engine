#include "single_thread_cache_pool.h"

template<typename T>
SingleThreadCachePool<T>::SingleThreadCachePool()
{
    for (std::size_t i = 0; i < POOL_SIZE; ++i)
    {
        m_objects[i].index = i;
        m_objects[i].use_time = 0;
        m_objects[i].is_active = false;

        m_free_objects[i] = &m_objects[i];
    }
}

template<typename T>
SingleThreadCachePool<T>::Object* SingleThreadCachePool<T>::acquire()
{
    if (m_free_count == 0)
    {
        return nullptr;
    }

    Object* object = m_free_objects[m_head];

    ++m_head;

    if (m_head >= POOL_SIZE)
    {
        m_head = 0;
    }

    --m_free_count;

    object->is_active = true;

    ++object->use_time;

    return object;
}

template<typename T>
bool SingleThreadCachePool<T>::release(Object* object)
{
    if (object == nullptr)
    {
        return false;
    }

    if (!object->is_active)
    {
        return false;
    }

    object->is_active = false;

    m_free_objects[m_tail] = object;

    ++m_tail;

    if (m_tail >= POOL_SIZE)
    {
        m_tail = 0;
    }

    ++m_free_count;

    return true;
}

template<typename T>
bool SingleThreadCachePool<T>::empty() const
{
    return m_free_count == 0;
}

template<typename T>
bool SingleThreadCachePool<T>::full() const
{
    return m_free_count == POOL_SIZE;
}

template<typename T>
bool SingleThreadCachePool<T>::is_active(Id id) const
{
    if (id.index >= POOL_SIZE)
    {
        return false;
    }

    const Object& object = m_objects[id.index];
    return object.is_active && object.use_time == id.use_time;
}

template<typename T>
std::size_t SingleThreadCachePool<T>::available_size() const
{
    return m_free_count;
}

template<typename T>
std::size_t SingleThreadCachePool<T>::active_size() const
{
    return POOL_SIZE - m_free_count;
}