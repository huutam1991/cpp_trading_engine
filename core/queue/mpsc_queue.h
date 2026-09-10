#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <cxxabi.h>
#include <emmintrin.h>
#include <x86intrin.h>
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>
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
        alignas(64) std::atomic<size_t> max_size{0};
        alignas(64) size_t tail{0};
        alignas(64) std::atomic<size_t> published_tail{0};
        alignas(64) std::atomic<uint64_t> last_pop_tsc{0};
        alignas(64) std::atomic<pid_t> consumer_tid{0};

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

    [[noreturn]]
    __attribute__((noinline, cold))
    static void crash_mpsc_multiple_consumers_detected()
    {
        throw std::runtime_error("MPSC MULTIPLE CONSUMERS DETECTED");
    }

    [[noreturn]]
    __attribute__((noinline, cold))
    static void crash_mpsc_real_full_consumer_not_registered()
    {
        throw std::runtime_error("MPSC REAL FULL CONSUMER NOT REGISTERED");
    }

    [[noreturn]]
    __attribute__((noinline, cold))
    static void crash_mpsc_real_full_tgkill_failed()
    {
        throw std::runtime_error("MPSC REAL FULL TGKILL FAILED");
    }

    [[noreturn]]
    __attribute__((noinline, cold))
    static void crash_mpsc_real_full_tgkill_returned()
    {
        throw std::runtime_error("MPSC REAL FULL TGKILL RETURNED");
    }

    FORCE_INLINE static pid_t current_linux_tid()
    {
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }

    FORCE_INLINE void register_consumer_thread()
    {
        const pid_t tid = current_linux_tid();
        pid_t expected = 0;

        if (m_pool_buffer.consumer_tid.compare_exchange_strong(
                expected,
                tid,
                std::memory_order_relaxed,
                std::memory_order_relaxed))
        {
            return;
        }

        if (expected != tid)
        {
            crash_mpsc_multiple_consumers_detected();
        }
    }

    // On REAL FULL, crash the consumer thread itself so the crash reporter
    // captures the exact coroutine/business function currently running there.
    [[noreturn]]
    __attribute__((noinline, cold))
    void crash_consumer_thread_for_real_full()
    {
        const pid_t consumer =
            m_pool_buffer.consumer_tid.load(std::memory_order_relaxed);

        const pid_t current = current_linux_tid();

        if (consumer == 0)
        {
            crash_mpsc_real_full_consumer_not_registered();
        }

        if (consumer == current)
        {
            // Current thread already is the consumer. Preserve this exact stack.
            ::abort();
        }

        const long rc = ::syscall(
            SYS_tgkill,
            static_cast<pid_t>(::getpid()),
            consumer,
            SIGABRT);

        if (rc != 0)
        {
            crash_mpsc_real_full_tgkill_failed();
        }

        // Normally unreachable unless a custom SIGABRT handler returns.
        crash_mpsc_real_full_tgkill_returned();
    }

    // ------------------------------------------------------------------------
    // Stack-trace-only diagnostics for REAL FULL.
    //
    // We instantiate 1000 distinct noinline functions, one per elapsed-us
    // bucket since the consumer's last successful pop().
    //
    // Bucket N means approximately:
    //   N == 0   : last pop was < 1 us ago
    //   N == 1   : last pop was 1..2 us ago
    //   ...
    //   N == 998 : last pop was 998..999 us ago
    //   N == 999 : last pop was >= 999 us ago
    //
    // The function template argument appears directly in a demangled stack
    // trace, e.g.:
    //   crash_mpsc_real_full_last_pop_us_bucket<37ul>()
    //
    // 1000 buckets were chosen instead of 10000 to keep binary size and
    // compile/link cost reasonable while retaining 1-us resolution over the
    // interval that matters most for this queue.
    //
    // This constant matches the previously measured ~3.072 GHz invariant TSC.
    // If this binary runs on a machine with a materially different TSC rate,
    // adjust this value; queue correctness does not depend on it.
    // ------------------------------------------------------------------------
    static constexpr uint64_t TSC_TICKS_PER_US = 3'072ULL;
    static constexpr size_t REAL_FULL_TIME_BUCKET_COUNT = 1000;

    using RealFullCrashFn = void (*)();

    [[noreturn]]
    __attribute__((noinline, cold))
    static void crash_mpsc_real_full_before_first_pop()
    {
        throw std::runtime_error("MPSC REAL FULL BEFORE FIRST POP");
    }

    template <size_t ElapsedUsBucket>
    [[noreturn]]
    __attribute__((noinline, cold))
    static void crash_mpsc_real_full_last_pop_us_bucket()
    {
        // Keep ElapsedUsBucket observably used so LTO/ICF cannot trivially
        // merge all template instantiations into one identical function.
        throw std::runtime_error(
            "MPSC REAL FULL LAST POP US BUCKET " +
            std::to_string(ElapsedUsBucket)
        );
    }

    template <size_t... I>
    static constexpr std::array<RealFullCrashFn, sizeof...(I)>
    make_real_full_crash_table(std::index_sequence<I...>)
    {
        return {
            &crash_mpsc_real_full_last_pop_us_bucket<I>...
        };
    }

    inline static constexpr auto real_full_crash_table =
        make_real_full_crash_table(
            std::make_index_sequence<REAL_FULL_TIME_BUCKET_COUNT>{}
        );

    [[noreturn]]
    __attribute__((noinline, cold))
    void crash_mpsc_real_full_by_last_pop_time()
    {
        const uint64_t last_pop =
            m_pool_buffer.last_pop_tsc.load(std::memory_order_relaxed);

        if (last_pop == 0)
        {
            crash_mpsc_real_full_before_first_pop();
        }

        const uint64_t now = __rdtsc();
        const uint64_t elapsed_ticks = now - last_pop;
        const uint64_t elapsed_us = elapsed_ticks / TSC_TICKS_PER_US;

        const size_t bucket =
            elapsed_us >= REAL_FULL_TIME_BUCKET_COUNT - 1
                ? REAL_FULL_TIME_BUCKET_COUNT - 1
                : static_cast<size_t>(elapsed_us);

        real_full_crash_table[bucket]();

        __builtin_unreachable();
    }

    [[noreturn]]
    __attribute__((noinline, cold))
    static void crash_mpsc_false_full()
    {
        throw std::runtime_error("MPSC FALSE FULL");
    }

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

        // MeasureTime measure_time("MPSCQueue::push, name: " + name);

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

                    // m_pool_buffer.size.fetch_add(1, std::memory_order_release);

                    auto current = m_pool_buffer.size.fetch_add(1, std::memory_order_relaxed) + 1;
                    auto old_max = m_pool_buffer.max_size.load(std::memory_order_relaxed);
                    while (current > old_max &&
                        !m_pool_buffer.max_size.compare_exchange_weak(
                            old_max,
                            current,
                            std::memory_order_relaxed))
                    {
                    }

                    return;
                }
            }
            else if (diff < 0)
            {
                const size_t head =
                    m_pool_buffer.head.load(std::memory_order_relaxed);

                const size_t tail =
                    m_pool_buffer.published_tail.load(std::memory_order_relaxed);

                const size_t outstanding = head - tail;

                if (outstanding >= Size)
                {
                    crash_consumer_thread_for_real_full();
                }

                crash_mpsc_false_full();
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
        register_consumer_thread();
        // MeasureTime measure_time("MPSCQueue::pop, name: " + name);

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
            m_pool_buffer.published_tail.store(pos + 1, std::memory_order_relaxed);
            m_pool_buffer.last_pop_tsc.store(__rdtsc(), std::memory_order_relaxed);
            m_pool_buffer.size.fetch_sub(1, std::memory_order_relaxed);

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

    FORCE_INLINE size_t max_size()
    {
        return m_pool_buffer.max_size.load(std::memory_order_relaxed);
    }
};