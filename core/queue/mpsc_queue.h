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
#include <core_dump_diagnostics/gdb_support.h>

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

    // Keep the last 200 successful producer reservations. At queue capacity 40
    // this gives us exactly:
    //   - the 40 currently outstanding queue positions, and
    //   - up to 160 enqueue positions immediately before them.
    //
    // The timestamp is recorded immediately after a producer successfully
    // reserves a queue position by advancing head. This is the same ordering
    // domain used by the REAL FULL check (head - published_tail).
    static constexpr size_t ENQUEUE_HISTORY_CAPACITY = 200;
    static constexpr size_t ENQUEUE_HISTORY_PREVIOUS_COUNT = 160;

    struct EnqueueHistorySlot
    {
        // queue_position + 1. Zero means this history slot has never been used.
        std::atomic<uint64_t> queue_position_plus_one{0};
        std::atomic<uint64_t> tsc{0};
    };

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

        // Diagnostic-only ring. Indexed by queue position % 200.
        alignas(64) std::array<EnqueueHistorySlot, ENQUEUE_HISTORY_CAPACITY>
            enqueue_history{};

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

    FORCE_INLINE void record_enqueue_history(
        const size_t queue_position,
        const uint64_t enqueue_tsc)
    {
        EnqueueHistorySlot& history_slot =
            m_pool_buffer.enqueue_history[
                queue_position % ENQUEUE_HISTORY_CAPACITY];

        // Publish the timestamp first, then publish its queue-position tag.
        // A crash reader accepts the value only when the tag still matches the
        // exact queue position it asked for.
        history_slot.tsc.store(
            enqueue_tsc,
            std::memory_order_relaxed);

        history_slot.queue_position_plus_one.store(
            static_cast<uint64_t>(queue_position) + 1,
            std::memory_order_release);
    }

    uint64_t read_enqueue_history_tsc(
        const size_t queue_position) const
    {
        const EnqueueHistorySlot& history_slot =
            m_pool_buffer.enqueue_history[
                queue_position % ENQUEUE_HISTORY_CAPACITY];

        const uint64_t expected_position_plus_one =
            static_cast<uint64_t>(queue_position) + 1;

        const uint64_t position_before =
            history_slot.queue_position_plus_one.load(
                std::memory_order_acquire);

        if (position_before != expected_position_plus_one)
        {
            return 0;
        }

        const uint64_t enqueue_tsc =
            history_slot.tsc.load(std::memory_order_relaxed);

        // Re-read the tag so an overwrite of this ring slot while the producer
        // is building the crash snapshot cannot pair a new position with an old
        // timestamp (or vice versa).
        const uint64_t position_after =
            history_slot.queue_position_plus_one.load(
                std::memory_order_acquire);

        if (position_after != expected_position_plus_one)
        {
            return 0;
        }

        return enqueue_tsc;
    }

    static void append_enqueue_history_entry(
        std::string& output,
        const size_t queue_position,
        const uint64_t enqueue_tsc)
    {
        if (!output.empty())
        {
            output += ",";
        }

        // Format: queue_position:enqueue_tsc
        output += std::to_string(queue_position);
        output += ":";
        output += std::to_string(enqueue_tsc);
    }

    // Read a uint64_t value previously published through
    // KEEP_FOR_GDB_SHARE_BETWEEN_THREADS(). The shared diagnostic registry is
    // process-wide, so the producer can inspect the Mongo consumer's published
    // start timestamp without signaling or resuming the consumer.
    static uint64_t read_shared_gdb_uint64(const char* variable_name)
    {
        while (g_gdb_keep_shared_registry_lock.test_and_set(std::memory_order_acquire))
        {
            _mm_pause();
        }

        uint64_t value = 0;

        if (GdbKeepSharedEntry* entry =
                gdb_find_shared_entry_by_name(variable_name);
            entry != nullptr && entry->active)
        {
            char* end = nullptr;
            const unsigned long long parsed =
                std::strtoull(entry->value, &end, 10);

            if (end != entry->value)
            {
                value = static_cast<uint64_t>(parsed);
            }
        }

        g_gdb_keep_shared_registry_lock.clear(std::memory_order_release);
        return value;
    }

    // On REAL FULL, crash THIS producer immediately.
    //
    // Do not signal the consumer: by the time SIGABRT is delivered the consumer
    // may already have returned from the slow task and entered another task.
    //
    // MongoQuery::replace_one publishes mongo_replace_one_start_tsc while the
    // blocking Mongo call is active. The producer snapshots that shared value,
    // computes the elapsed ticks at REAL FULL, stores all diagnostics in its own
    // KEEP_FOR_GDB registry, and aborts immediately. The crash stack therefore
    // belongs to the producer, while the diagnostic variables tell us how long
    // the current replace_one had been active.
    [[noreturn]]
    GDB_DIAGNOSTIC_FUNCTION
    void crash_producer_thread_for_real_full()
    {
        _mm_lfence();
        const uint64_t mpsc_real_full_tsc = __rdtsc();

        const uint64_t mongo_replace_one_start_tsc =
            read_shared_gdb_uint64("mongo_replace_one_start_tsc");

        const bool mongo_replace_one_active =
            mongo_replace_one_start_tsc != 0;

        const uint64_t mongo_replace_one_elapsed_ticks =
            mongo_replace_one_active &&
            mpsc_real_full_tsc >= mongo_replace_one_start_tsc
                ? mpsc_real_full_tsc - mongo_replace_one_start_tsc
                : 0;

        const size_t mpsc_head =
            m_pool_buffer.head.load(std::memory_order_relaxed);

        const size_t mpsc_published_tail =
            m_pool_buffer.published_tail.load(std::memory_order_relaxed);

        const size_t mpsc_outstanding =
            mpsc_head - mpsc_published_tail;

        const size_t mpsc_size =
            m_pool_buffer.size.load(std::memory_order_relaxed);

        const std::string mpsc_queue_name = name;

        // Snapshot published by the consumer immediately after it popped the
        // task that is currently executing. If mongo_replace_one_active == true,
        // no later pop can have happened on this single-consumer queue, so this
        // is the queue occupancy at the start of the current consumer task.
        const uint64_t consumer_task_start_queue_size =
            read_shared_gdb_uint64("consumer_task_start_queue_size");

        // --------------------------------------------------------------------
        // Enqueue history snapshot.
        //
        // At REAL FULL with Size == 40:
        //   [mpsc_published_tail, mpsc_head)
        //       = the 40 queue positions currently outstanding.
        //
        // The 160 positions immediately before mpsc_published_tail are the
        // previous enqueue events, giving a full 200-position history window.
        //
        // Each serialized entry is:
        //     queue_position:enqueue_tsc
        // --------------------------------------------------------------------
        std::string enqueue_tsc_current_outstanding;
        std::string enqueue_tsc_previous_160;

        enqueue_tsc_current_outstanding.reserve(2048);
        enqueue_tsc_previous_160.reserve(8192);

        size_t current_outstanding_history_count = 0;
        size_t current_outstanding_after_mongo_start_count = 0;

        uint64_t current_outstanding_oldest_tsc = 0;
        uint64_t current_outstanding_newest_tsc = 0;

        for (size_t queue_position = mpsc_published_tail;
             queue_position < mpsc_head;
             ++queue_position)
        {
            const uint64_t enqueue_tsc =
                read_enqueue_history_tsc(queue_position);

            if (enqueue_tsc == 0 ||
                enqueue_tsc > mpsc_real_full_tsc)
            {
                continue;
            }

            append_enqueue_history_entry(
                enqueue_tsc_current_outstanding,
                queue_position,
                enqueue_tsc);

            if (current_outstanding_history_count == 0 ||
                enqueue_tsc < current_outstanding_oldest_tsc)
            {
                current_outstanding_oldest_tsc = enqueue_tsc;
            }

            if (enqueue_tsc > current_outstanding_newest_tsc)
            {
                current_outstanding_newest_tsc = enqueue_tsc;
            }

            ++current_outstanding_history_count;

            if (mongo_replace_one_active &&
                enqueue_tsc >= mongo_replace_one_start_tsc)
            {
                ++current_outstanding_after_mongo_start_count;
            }
        }

        const uint64_t current_outstanding_span_ticks =
            current_outstanding_history_count >= 2 &&
            current_outstanding_newest_tsc >= current_outstanding_oldest_tsc
                ? current_outstanding_newest_tsc -
                    current_outstanding_oldest_tsc
                : 0;

        const size_t previous_history_end =
            mpsc_published_tail;

        const size_t previous_history_begin =
            previous_history_end > ENQUEUE_HISTORY_PREVIOUS_COUNT
                ? previous_history_end - ENQUEUE_HISTORY_PREVIOUS_COUNT
                : 0;

        size_t previous_160_history_count = 0;
        uint64_t previous_160_oldest_tsc = 0;
        uint64_t previous_160_newest_tsc = 0;

        for (size_t queue_position = previous_history_begin;
             queue_position < previous_history_end;
             ++queue_position)
        {
            const uint64_t enqueue_tsc =
                read_enqueue_history_tsc(queue_position);

            if (enqueue_tsc == 0 ||
                enqueue_tsc > mpsc_real_full_tsc)
            {
                continue;
            }

            append_enqueue_history_entry(
                enqueue_tsc_previous_160,
                queue_position,
                enqueue_tsc);

            if (previous_160_history_count == 0 ||
                enqueue_tsc < previous_160_oldest_tsc)
            {
                previous_160_oldest_tsc = enqueue_tsc;
            }

            if (enqueue_tsc > previous_160_newest_tsc)
            {
                previous_160_newest_tsc = enqueue_tsc;
            }

            ++previous_160_history_count;
        }

        const uint64_t previous_160_span_ticks =
            previous_160_history_count >= 2 &&
            previous_160_newest_tsc >= previous_160_oldest_tsc
                ? previous_160_newest_tsc -
                    previous_160_oldest_tsc
                : 0;

        const size_t enqueue_history_valid_count =
            current_outstanding_history_count +
            previous_160_history_count;

        KEEP_FOR_GDB(mpsc_real_full_tsc);
        KEEP_FOR_GDB(mongo_replace_one_start_tsc);
        KEEP_FOR_GDB(consumer_task_start_queue_size);
        KEEP_FOR_GDB(mongo_replace_one_active);
        KEEP_FOR_GDB(mongo_replace_one_elapsed_ticks);
        KEEP_FOR_GDB(mpsc_head);
        KEEP_FOR_GDB(mpsc_published_tail);
        KEEP_FOR_GDB(mpsc_outstanding);
        KEEP_FOR_GDB(mpsc_size);
        KEEP_FOR_GDB(mpsc_queue_name);

        KEEP_FOR_GDB(enqueue_history_valid_count);

        KEEP_FOR_GDB(current_outstanding_history_count);
        KEEP_FOR_GDB(current_outstanding_after_mongo_start_count);
        KEEP_FOR_GDB(current_outstanding_oldest_tsc);
        KEEP_FOR_GDB(current_outstanding_newest_tsc);
        KEEP_FOR_GDB(current_outstanding_span_ticks);
        KEEP_FOR_GDB(enqueue_tsc_current_outstanding);

        KEEP_FOR_GDB(previous_160_history_count);
        KEEP_FOR_GDB(previous_160_oldest_tsc);
        KEEP_FOR_GDB(previous_160_newest_tsc);
        KEEP_FOR_GDB(previous_160_span_ticks);
        KEEP_FOR_GDB(enqueue_tsc_previous_160);

        // Crash immediately on the producer thread. No tgkill/SIGABRT handoff
        // to the consumer, so there is no opportunity for the consumer stack to
        // move to another task before the core is taken.
        ::abort();
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
                    // pos is now this enqueue's unique global queue position.
                    // Record its TSC immediately after reservation so the last
                    // 200 successful enqueue positions can be reconstructed at
                    // REAL FULL.
                    _mm_lfence();
                    const uint64_t enqueue_tsc = __rdtsc();
                    record_enqueue_history(pos, enqueue_tsc);

                    slot.value = std::move(item);

                    // Update the diagnostic occupancy counter BEFORE publishing
                    // the slot. The consumer only sees the item after the
                    // release-store below, so its matching fetch_sub() cannot
                    // race ahead of this increment.
                    const size_t current =
                        m_pool_buffer.size.fetch_add(
                            1,
                            std::memory_order_relaxed) + 1;

                    auto old_max =
                        m_pool_buffer.max_size.load(std::memory_order_relaxed);

                    while (current > old_max &&
                        !m_pool_buffer.max_size.compare_exchange_weak(
                            old_max,
                            current,
                            std::memory_order_relaxed))
                    {
                    }

                    // Publish item only after value + diagnostic size are ready.
                    slot.sequence.store(pos + 1, std::memory_order_release);

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
                    crash_producer_thread_for_real_full();
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

            // fetch_sub() returns the occupancy immediately BEFORE this pop.
            // Subtracting one therefore gives the occupancy immediately AFTER
            // this exact pop, without a second load that could observe producer
            // pushes that happened later.
            const size_t size_before_pop =
                m_pool_buffer.size.fetch_sub(
                    1,
                    std::memory_order_relaxed);

            const size_t consumer_task_start_queue_size =
                size_before_pop - 1;

            KEEP_FOR_GDB_SHARE_BETWEEN_THREADS(
                consumer_task_start_queue_size);

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