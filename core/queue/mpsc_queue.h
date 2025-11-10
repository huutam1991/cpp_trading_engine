#pragma once

#include <cxxabi.h>
#include <cstddef>
#include <string>
#include <array>
#include <atomic>
#include <emmintrin.h>

#include <time/measure_time.h>

#define FORCE_INLINE inline __attribute__((always_inline))

template <typename T>
std::string demangled_get_name()
{
    int status;
    char* realname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    std::string result = (status == 0 && realname) ? realname : typeid(T).name();
    std::free(realname);
    return result;
}

template <typename T, typename U = void>
struct GetTypeName
{
    static std::string get_name()
    {
        return demangled_get_name<T>();
    }
};

template <typename T>
struct GetTypeName<T, std::void_t<decltype(T::get_name())>>
{
    static std::string get_name()
    {
        return T::get_name();
    }
};

template <class T, size_t Size>
class MPSCQueue
{
    struct alignas(64) ObjectPointerWrapper
    {
        std::atomic<T*> ptr;
    };

    struct PoolBuffer
    {
        alignas(64) std::array<ObjectPointerWrapper, Size> available_items;
        alignas(64) std::atomic<size_t> head = 0;
        alignas(64) size_t              tail = 0;
        alignas(64) std::atomic<size_t> size = 0;

        PoolBuffer()
        {
            for (size_t i = 0; i < Size; ++i)
            {
                available_items[i].ptr.store(nullptr, std::memory_order_relaxed);
            }
        }

        FORCE_INLINE size_t get_current_head()
        {
            size_t current = head.load(std::memory_order_acquire);
            size_t next = (current + 1) % Size;

            while (!head.compare_exchange_weak(current, next,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed))
            {
                next = (current + 1) % Size;
            }

            return current;
        }
    };

    PoolBuffer m_pool_buffer;
    std::string name = GetTypeName<T>::get_name();

public:
    // push an item into the queue
    FORCE_INLINE void push(T* item)
    {
        if (item != nullptr)
        {
            // MeasureTime measure_time("MPSCQueue::push, name: " + name, MeasureUnit::NANOSECOND);
            // MeasureTime measure_time("MPSCQueue::push", MeasureUnit::NANOSECOND);

            if (m_pool_buffer.size.load(std::memory_order_acquire) == Size)
            {
                throw std::runtime_error("Queue is full: [" + name + "]");
            }

            // Push item into the pool
            size_t head_index = m_pool_buffer.get_current_head();
            m_pool_buffer.available_items[head_index].ptr.store(item, std::memory_order_release);

            // Increase size only after successfully moving head
            m_pool_buffer.size.fetch_add(1, std::memory_order_release);
        }
        else
        {
            throw std::runtime_error("Attempt to release a null item back to the cache pool: [" + name + "]");
        }
    }

    // Release a cache item back to the pool
    FORCE_INLINE T* pop()
    {
        if (m_pool_buffer.size.load(std::memory_order_acquire) == 0)
        {
            return nullptr;
        }

        // Pop item from the pool
        size_t tail_index = m_pool_buffer.tail;
        T* item = m_pool_buffer.available_items[tail_index].ptr.load(std::memory_order_acquire);
        if (item == nullptr)
        {
            _mm_pause();
            return nullptr;
        }

        // MeasureTime measure_time("MPSCQueue::pop, name: " + name, MeasureUnit::NANOSECOND);
        // MeasureTime measure_time("MPSCQueue::release", MeasureUnit::NANOSECOND);

        m_pool_buffer.available_items[tail_index].ptr.store(nullptr, std::memory_order_release);
        m_pool_buffer.tail = (tail_index + 1) % Size;

        // Decrease size only after successfully moving tail
        m_pool_buffer.size.fetch_sub(1, std::memory_order_release);

        return item;
    }

    FORCE_INLINE size_t head()
    {
        return m_pool_buffer.head.load(std::memory_order_relaxed);
    }

    FORCE_INLINE size_t tail()
    {
        return m_pool_buffer.tail.load(std::memory_order_relaxed);
    }

    FORCE_INLINE size_t size()
    {
        return m_pool_buffer.size.load(std::memory_order_relaxed);
    }
};