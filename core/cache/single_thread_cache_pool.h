#pragma once

#include <array>
#include <cstddef>

#include <utils/type_name.h>

template<typename T, size_t Size>
class SingleThreadCachePool
{
public:

    struct Id
    {
        std::size_t index;
        std::size_t use_time;
    };

    struct Object
    {
        T value{};

        std::size_t index = 0;
        std::size_t use_time = 0;

        bool is_active = false;

        Id get_id() const
        {
            return Id
            {
                index,
                use_time
            };
        }
    };

    template <typename U>
    static constexpr bool has_init = requires(U& obj)
    {
        obj.init();
    };

    template <typename U>
    static constexpr bool has_refresh = requires(U& obj)
    {
        obj.refresh();
    };

public:
    SingleThreadCachePool();

    Object* acquire();
    bool release(Object* object);
    bool empty() const;
    bool full() const;
    bool is_active(Id id) const;

    std::size_t available_size() const;
    std::size_t active_size() const;

private:
    static constexpr std::size_t POOL_SIZE = Size;
    std::string name = type_name::TypeName<T>::name();

    std::array<Object, POOL_SIZE> m_objects;
    std::array<Object*, POOL_SIZE> m_free_objects;

    std::size_t m_head = 0;
    std::size_t m_tail = 0;
    std::size_t m_free_count = POOL_SIZE;
};

template<typename T, size_t Size>
SingleThreadCachePool<T, Size>::SingleThreadCachePool()
{
    for (std::size_t i = 0; i < POOL_SIZE; ++i)
    {
        m_objects[i].index = i;
        m_objects[i].use_time = 0;
        m_objects[i].is_active = false;

        m_free_objects[i] = &m_objects[i];
    }
}

template<typename T, size_t Size>
typename SingleThreadCachePool<T, Size>::Object* SingleThreadCachePool<T, Size>::acquire()
{
    if (m_free_count == 0)
    {
        throw std::runtime_error("SingleThreadCachePool<" + name + "> - [acquire] No available items in cache pool");
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

    // Check if the item has init method and call it
    if constexpr (has_init<T>)
    {
        object->value.init();
    }

    return object;
}

template<typename T, size_t Size>
bool SingleThreadCachePool<T, Size>::release(Object* object)
{
    if (object == nullptr)
    {
        throw std::runtime_error("SingleThreadCachePool<" + name + "> - [release] Attempting to release a nullptr object");
    }

    if (!object->is_active)
    {
        throw std::runtime_error("SingleThreadCachePool<" + name + "> - [release] Attempting to release an inactive object");
    }

    object->is_active = false;
    m_free_objects[m_tail] = object;

    // Check if the item has refresh method and call it
    if constexpr (has_refresh<T>)
    {
        object->value.refresh();
    }

    ++m_tail;
    if (m_tail >= POOL_SIZE)
    {
        m_tail = 0;
    }

    ++m_free_count;

    return true;
}

template<typename T, size_t Size>
bool SingleThreadCachePool<T, Size>::empty() const
{
    return m_free_count == 0;
}

template<typename T, size_t Size>
bool SingleThreadCachePool<T, Size>::full() const
{
    return m_free_count == POOL_SIZE;
}

template<typename T, size_t Size>
bool SingleThreadCachePool<T, Size>::is_active(Id id) const
{
    if (id.index >= POOL_SIZE)
    {
        return false;
    }

    const Object& object = m_objects[id.index];
    return object.is_active && object.use_time == id.use_time;
}

template<typename T, size_t Size>
std::size_t SingleThreadCachePool<T, Size>::available_size() const
{
    return m_free_count;
}

template<typename T, size_t Size>
std::size_t SingleThreadCachePool<T, Size>::active_size() const
{
    return POOL_SIZE - m_free_count;
}