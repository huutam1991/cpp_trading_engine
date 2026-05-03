#pragma once

#include <cxxabi.h>
#include <cstddef>
#include <string>
#include <array>
#include <atomic>

#include <time/measure_time.h>
#include <utils/type_name.h>

#define FORCE_INLINE inline __attribute__((always_inline))

template <class T, size_t Size>
class SharedCachePool
{
    struct alignas(64) ObjectWrapper
    {
        alignas(64) std::atomic<size_t> reference_counter = 0;
        T object;
    };

    struct alignas(64) ObjectPointerWrapper
    {
        ObjectWrapper* object;
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
    class ObjectPointer
    {
        T* object;
        ObjectWrapper* wrapper;

    public:
        std::atomic<size_t>* reference_counter;

        ObjectPointer(ObjectWrapper* p) : object(&p->object), reference_counter(&p->reference_counter), wrapper(p)
        {
            reference_counter->store(1, std::memory_order_release);
        }

        ObjectPointer(const ObjectPointer& other) : object(other.object), reference_counter(other.reference_counter), wrapper(other.wrapper)
        {
            if (reference_counter != nullptr)
            {
                reference_counter->fetch_add(1, std::memory_order_acq_rel);
            }
        }

        ObjectPointer(ObjectPointer&& other) noexcept
            :   object(other.object),
                reference_counter(other.reference_counter),
                wrapper(other.wrapper)
        {
            other.object = nullptr;
            other.reference_counter = nullptr;
            other.wrapper = nullptr;
        }

        ~ObjectPointer()
        {
            if (reference_counter != nullptr && wrapper != nullptr)
            {
                if (reference_counter->fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    // Last reference, release the object back to the pool
                    SharedCachePool::release(wrapper);
                }
            }
        }

        ObjectPointer& operator=(const ObjectPointer& other)
        {
            if (this != &other)
            {
                // Decrease reference count of current object
                if (reference_counter != nullptr && reference_counter->fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    SharedCachePool::release(wrapper);
                }

                // Copy new pointer and reference counter
                object = other.object;
                reference_counter = other.reference_counter;
                wrapper = other.wrapper;

                // Increase reference count of new object
                if (reference_counter != nullptr)
                {
                    reference_counter->fetch_add(1, std::memory_order_acq_rel);
                }
            }
            return *this;
        }

        ObjectPointer& operator=(ObjectPointer&& other) noexcept
        {
            if (this != &other)
            {
                if (reference_counter != nullptr && reference_counter->fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    SharedCachePool::release(wrapper);
                }

                object = other.object;
                reference_counter = other.reference_counter;
                wrapper = other.wrapper;

                other.object = nullptr;
                other.reference_counter = nullptr;
                other.wrapper = nullptr;
            }

            return *this;
        }

        T* get()
        {
            return object;
        }

        const T* get() const
        {
            return object;
        }

        T* operator->()
        {
            return object;
        }

        const T* operator->() const
        {
            return object;
        }

        T& operator*()
        {
            return *object;
        }

        const T& operator*() const
        {
            return *object;
        }

        operator bool() const
        {
            return object != nullptr;
        }

        bool operator==(std::nullptr_t) const
        {
            return object == nullptr;
        }
    };

private:
    struct PoolBuffer
    {
        alignas(64) std::array<ObjectPointerWrapper, Size> available_items;
        alignas(64) std::array<ObjectWrapper, Size> data;
        alignas(64) std::atomic<size_t> head = 0;
        alignas(64) std::atomic<size_t> tail = 0;
        alignas(64) std::atomic<size_t> size = Size;

        PoolBuffer()
        {
            for (size_t i = 0; i < Size; ++i)
            {
                available_items[i].object = &data[i];
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
    // Acquire a cache item
    FORCE_INLINE static ObjectPointer acquire()
    {
        static std::string name = type_name::TypeName<T>::name();

        // MeasureTime measure_time("SharedCachePool::acquire, name: " + name, MeasureUnit::NANOSECOND);

        PoolBuffer& pool_buffer = get_pool_buffer();
        if (pool_buffer.size.load(std::memory_order_relaxed) == 0)
        {
            throw std::runtime_error("No available items in cache pool: [" + name + "]");
        }

        // Get the item from the pool
        size_t head_index = pool_buffer.get_current_head();
        ObjectPointer item = pool_buffer.available_items[head_index].object;

        // Decrease size only after successfully moving head
        pool_buffer.size.fetch_sub(1, std::memory_order_release);

        // Check if the item has init method and call it
        if constexpr (has_init<T>)
        {
            item.object->init();
        }

        return item;
    }

    // Release a cache item back to the pool
    FORCE_INLINE static void release(ObjectWrapper* item)
    {
        static std::string name = type_name::TypeName<T>::name();

        if (item != nullptr)
        {
            {
                // MeasureTime measure_time("SharedCachePool::release, name: " + name, MeasureUnit::NANOSECOND);

                // Check if the item has refresh method and call it
                if constexpr (has_refresh<T>)
                {
                    item->object.refresh();
                }

                // Add item back to the pool
                PoolBuffer& pool_buffer = get_pool_buffer();
                size_t tail_index = pool_buffer.get_current_tail();
                pool_buffer.available_items[tail_index].object = item;

                // Increase size only after successfully moving tail
                pool_buffer.size.fetch_add(1, std::memory_order_release);
            }
        }
        else
        {
            throw std::runtime_error("Attempt to release a null wrapper back to the cache pool: [" + name + "]");
        }
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

    FORCE_INLINE static size_t total_released_items()
    {
        size_t size = get_pool_buffer().size.load(std::memory_order_acquire);
        return Size - size;
    }
};