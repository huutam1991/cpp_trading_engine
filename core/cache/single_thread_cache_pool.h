#pragma once

#include <array>
#include <cstddef>

#define MAX_POOL_SIZE 10000

template<typename T>
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
    static constexpr std::size_t POOL_SIZE = MAX_POOL_SIZE;

    std::array<Object, POOL_SIZE> m_objects;
    std::array<Object*, POOL_SIZE> m_free_objects;

    std::size_t m_head = 0;
    std::size_t m_tail = 0;
    std::size_t m_free_count = POOL_SIZE;
};