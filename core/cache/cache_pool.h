#pragma once

#include <cxxabi.h>
#include <cstddef>
#include <string>
#include <array>

#include <utils/util_macros.h>
#include <utils/spin_lock.h>
#include <time/measure_time.h>

template <typename T>
std::string demangled_name()
{
    int status;
    char* realname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    std::string result = (status == 0 && realname) ? realname : typeid(T).name();
    std::free(realname);
    return result;
}

template <typename T, typename = void>
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
class CachePool
{
    struct PoolBuffer
    {
        std::array<T*, Size> available_items;
        std::array<T, Size> data;
        size_t head = 0;
        size_t tail = Size - 1;
        size_t size = Size;

        PoolBuffer()
        {
            for (size_t i = 0; i < Size; ++i)
            {
                available_items[i] = &data[i];
            }
        }

        FORCE_INLINE void move_head()
        {
            head++;
            if (head >= Size)
            {
                head = 0; // cycle the head index
            }
        }

        FORCE_INLINE void move_tail()
        {
            tail++;
            if (tail >= Size)
            {
                tail = 0; // cycle the tail index
            }
        }
    };

    FORCE_INLINE static PoolBuffer& get_pool_buffer()
    {
        static PoolBuffer* pool_buffer = new PoolBuffer();
        return *pool_buffer;
    }

    FORCE_INLINE static SpinLock& get_spin_lock()
    {
        static SpinLock spin_lock;
        return spin_lock;
    }

public:
    // Acquire a cache item
    FORCE_INLINE static T* acquire()
    {
        // MeasureTime measure_time("CachePool::acquire", MeasureUnit::NANOSECOND);
        std::lock_guard<SpinLock> guard(get_spin_lock());

        PoolBuffer& pool_buffer = get_pool_buffer();
        if (pool_buffer.size == 0)
        {
            throw std::runtime_error("No available items in cache pool: [" + TypeName<T>::name() + "]");
        }

        T* item = pool_buffer.available_items[pool_buffer.head];
        pool_buffer.move_head();
        pool_buffer.size--;

        return item;
    }

    // Release a cache item back to the pool
    FORCE_INLINE static void release(T* item)
    {
        // MeasureTime measure_time("CachePool::release", MeasureUnit::NANOSECOND);
        std::lock_guard<SpinLock> guard(get_spin_lock());

        PoolBuffer& pool_buffer = get_pool_buffer();
        if (item != nullptr)
        {
            pool_buffer.move_tail();
            pool_buffer.available_items[pool_buffer.tail] = item;
            pool_buffer.size++;
        }
        else
        {
            throw std::runtime_error("Attempt to release a null item back to the cache pool: [" + TypeName<T>::name() + "]");
        }
    }

    FORCE_INLINE static size_t size()
    {
        std::lock_guard<SpinLock> guard(get_spin_lock());
        return get_pool_buffer().size;
    }
};