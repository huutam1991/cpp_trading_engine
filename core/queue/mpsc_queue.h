#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <cxxabi.h>
#include <emmintrin.h>
#include <type_traits>
#include <utility>

#include <time/measure_time.h>

#define FORCE_INLINE inline __attribute__((always_inline))

template <typename T>
std::string demangled_get_name()
{
    int status = 0;
    char* realname = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
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

template<typename T, typename = void>
struct SupportsNullptr : std::false_type {};

template<typename T>
struct SupportsNullptr<
    T,
    std::void_t<
        decltype(T{nullptr}),
        decltype(std::declval<T&>() = nullptr),
        decltype(std::declval<T>() == nullptr)
    >
> : std::true_type {};

template <class T, size_t Size>
class MPSCQueue
{
    static_assert(Size > 1, "MPSCQueue Size must be > 1");
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
    static_assert(
        std::is_pointer_v<T> || SupportsNullptr<T>::value,
        "T must either be a pointer type or support construction/comparison with nullptr"
    );

    struct alignas(64) Slot
    {
        std::atomic<size_t> sequence;
        T value{};
    };

    struct PoolBuffer
    {
        alignas(64) std::array<Slot, Size> available_items;
        alignas(64) std::atomic<size_t> head{0};
        alignas(64) std::atomic<size_t> size{0};
        alignas(64) size_t tail{0};

        PoolBuffer()
        {
            for (size_t i = 0; i < Size; ++i)
            {
                available_items[i].sequence.store(i, std::memory_order_relaxed);
                available_items[i].value = nullptr;
            }
        }
    };

    PoolBuffer m_pool_buffer;
    std::string name = GetTypeName<T>::get_name();

public:
    FORCE_INLINE void push(T item)
    {
        if constexpr (std::is_pointer_v<T>)
        {
            if (item == nullptr)
            {
                throw std::runtime_error
                (
                    "Attempt to release a null item back to the cache pool: [" + name + "]"
                );
            }
        }

        // MeasureTime measure_time("MPSCQueue::push, name: " + name, MeasureUnit::NANOSECOND);

        size_t pos = m_pool_buffer.head.load(std::memory_order_relaxed);

        while (true)
        {
            Slot& slot = m_pool_buffer.available_items[pos % Size];

            size_t seq = slot.sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

            if (diff == 0)
            {
                if (m_pool_buffer.head.compare_exchange_weak(
                        pos,
                        pos + 1,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed))
                {
                    slot.value = std::move(item);

                    // publish item
                    slot.sequence.store(pos + 1, std::memory_order_release);

                    m_pool_buffer.size.fetch_add(1, std::memory_order_release);
                    return;
                }
            }
            else if (diff < 0)
            {
                throw std::runtime_error("Queue is full: [" + name + "]");
            }
            else
            {
                pos = m_pool_buffer.head.load(std::memory_order_relaxed);
            }

            _mm_pause();
        }
    }

    FORCE_INLINE T pop()
    {
        // MeasureTime measure_time("MPSCQueue::pop, name: " + name, MeasureUnit::NANOSECOND);

        size_t pos = m_pool_buffer.tail;
        Slot& slot = m_pool_buffer.available_items[pos % Size];

        size_t seq = slot.sequence.load(std::memory_order_acquire);
        intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

        if (diff == 0)
        {
            T item = std::move(slot.value);
            slot.value = nullptr;

            // mark slot free for next producer round
            slot.sequence.store(pos + Size, std::memory_order_release);

            m_pool_buffer.tail = pos + 1;
            m_pool_buffer.size.fetch_sub(1, std::memory_order_release);

            return item;
        }

        return nullptr;
    }

    FORCE_INLINE size_t head()
    {
        return m_pool_buffer.head.load(std::memory_order_relaxed);
    }

    FORCE_INLINE size_t tail()
    {
        return m_pool_buffer.tail;
    }

    FORCE_INLINE size_t size()
    {
        return m_pool_buffer.size.load(std::memory_order_relaxed);
    }
};