#pragma once

#include <cxxabi.h>
#include <cstddef>
#include <string>
#include <array>
#include <atomic>

#include <time/measure_time.h>

#define FORCE_INLINE inline __attribute__((always_inline))

template <typename T>
std::string demangled_name()
    {
        int status;
        char* realname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
        std::string result = (status == 0 && realname) ? realname : typeid(T).name();
        std::free(realname);
        return result;
    }

template <typename T, typename U = void>
    struct TypeName
    {
    static std::string name()
        {
        return demangled_name<T>();
        }
    };

template <typename T>
struct TypeName<T, std::void_t<decltype(T::name())>>
    {
    static std::string name()
        {
            return T::name();
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
        alignas(64) std::atomic<size_t> tail = 0;
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

        FORCE_INLINE size_t get_current_tail()
        {
            size_t current = tail.load(std::memory_order_acquire);
            size_t next = (current + 1) % Size;

            while (!tail.compare_exchange_weak(current, next,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed))
            {
                next = (current + 1) % Size;
            }

            return current;
        }
    };

    FORCE_INLINE static PoolBuffer& get_pool_buffer()
    {
        static PoolBuffer* pool_buffer = new PoolBuffer();
        return *pool_buffer;
    }

public:
    // push an item into the queue
    FORCE_INLINE static void push(T* item)
    {
        if (item != nullptr)
        {
            // MeasureTime measure_time("MPSCQueue::push, name: " + TypeName<T>::get_name(), MeasureUnit::NANOSECOND);
            // MeasureTime measure_time("MPSCQueue::push", MeasureUnit::NANOSECOND);

            PoolBuffer& pool_buffer = get_pool_buffer();
            if (pool_buffer.size.load(std::memory_order_relaxed) == Size)
            {
                throw std::runtime_error("Queue is full: [" + TypeName<T>::get_name() + "]");
            }

            // Push item into the pool
            size_t head_index = pool_buffer.get_current_head();
            pool_buffer.available_items[head_index].ptr.store(item, std::memory_order_release);

            // Increase size only after successfully moving head
            pool_buffer.size.fetch_add(1, std::memory_order_release);
        }
        else
        {
            throw std::runtime_error("Attempt to release a null item back to the cache pool: [" + TypeName<T>::name() + "]");
        }
    }

    // Release a cache item back to the pool
    FORCE_INLINE static T* pop()
    {
        PoolBuffer& pool_buffer = get_pool_buffer();
        if (pool_buffer.size.load(std::memory_order_acquire) == 0)
        {
            return nullptr;
        }

        // MeasureTime measure_time("MPSCQueue::release, name: " + TypeName<T>::get_name(), MeasureUnit::NANOSECOND);
        // MeasureTime measure_time("MPSCQueue::release", MeasureUnit::NANOSECOND);

        // Pop imtem from the pool
        size_t tail_index = pool_buffer.get_current_tail();
        T* item;
        while ((item = pool_buffer.available_items[tail_index].ptr.load(std::memory_order_acquire)) == nullptr)
        {
            // Busy wait
        }
        pool_buffer.available_items[tail_index].ptr.store(nullptr, std::memory_order_release);

        // Decrease size only after successfully moving tail
        pool_buffer.size.fetch_sub(1, std::memory_order_release);

        return item;
    }

    FORCE_INLINE static size_t head()
    {
        return get_pool_buffer().head.load(std::memory_order_relaxed);
    }

    FORCE_INLINE static size_t tail()
    {
        return get_pool_buffer().tail.load(std::memory_order_relaxed);
    }

    FORCE_INLINE static size_t size()
    {
        return get_pool_buffer().size.load(std::memory_order_relaxed);
    }
};