#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

// Keep a real, non-inlined frame for functions whose state is intentionally
// preserved for post-mortem GDB/core-dump inspection.
#define GDB_DIAGNOSTIC_FUNCTION \
    __attribute__((noinline, optimize("O0", "no-omit-frame-pointer")))

inline constexpr std::size_t GDB_KEEP_MAX_VARIABLES = 32;
inline constexpr std::size_t GDB_KEEP_NAME_CAPACITY = 128;
inline constexpr std::size_t GDB_KEEP_FILE_CAPACITY = 512;
inline constexpr std::size_t GDB_KEEP_VALUE_CAPACITY = 64 * 1024;

struct GdbKeepEntry
{
    std::uint8_t active = 0;
    std::uint32_t line = 0;

    char name[GDB_KEEP_NAME_CAPACITY]{};
    char file[GDB_KEEP_FILE_CAPACITY]{};
    char value[GDB_KEEP_VALUE_CAPACITY]{};
};

struct GdbKeepRegistry
{
    // Physical stack frame currently owning the snapshot.
    std::uintptr_t owner_frame = 0;

    // Incremented for every NEW function invocation observed by KEEP_FOR_GDB,
    // even when the OS/compiler reuses exactly the same stack-frame address.
    std::uint64_t generation = 0;

    // True while at least one guard belonging to the current generation is
    // alive. When the last guard is destroyed this becomes false, but the
    // captured values are intentionally NOT cleared.
    std::uint8_t invocation_active = 0;

    // Number of KEEP_FOR_GDB guards alive in the current generation.
    std::uint32_t active_guards = 0;

    std::array<GdbKeepEntry, GDB_KEEP_MAX_VARIABLES> entries{};

    void clear_entries() noexcept
    {
        for (auto& entry : entries)
        {
            entry.active = 0;
            entry.line = 0;
            entry.name[0] = '\0';
            entry.file[0] = '\0';
            entry.value[0] = '\0';
        }
    }

    void begin_new_invocation(const void* frame) noexcept
    {
        owner_frame = reinterpret_cast<std::uintptr_t>(frame);

        // Important: this increments even if owner_frame happens to be equal
        // to the previous invocation's frame address.
        ++generation;

        invocation_active = 1;
        active_guards = 0;
        clear_entries();
    }
};

// One registry per OS thread. Values are copied into fixed POD buffers so GDB
// can read them directly from a core dump without depending on coroutine-local
// DWARF layout or libstdc++ pretty printers.
inline thread_local GdbKeepRegistry g_gdb_keep_registry __attribute__((used));

// Non-TLS bridge used only by post-mortem GDB/core-dump inspection.
// KEEP_FOR_GDB publishes the address of the current thread's TLS registry here
// so GDB does not need to resolve a thread_local symbol from the core file.
inline GdbKeepRegistry* g_gdb_keep_registry_for_core __attribute__((used)) = nullptr;

inline void gdb_publish_keep_registry_for_core() noexcept
{
    __atomic_store_n(
        &g_gdb_keep_registry_for_core,
        &g_gdb_keep_registry,
        __ATOMIC_RELEASE);

    asm volatile(""
                 :
                 : "m"(g_gdb_keep_registry_for_core),
                   "m"(g_gdb_keep_registry)
                 : "memory");
}

inline void gdb_copy_text(
    char* dst,
    std::size_t capacity,
    std::string_view src) noexcept
{
    if (capacity == 0)
    {
        return;
    }

    const std::size_t n = src.size() < capacity - 1
        ? src.size()
        : capacity - 1;

    for (std::size_t i = 0; i < n; ++i)
    {
        dst[i] = src[i];
    }

    dst[n] = '\0';
}

inline void gdb_copy_escaped_value(
    char* dst,
    std::size_t capacity,
    std::string_view src) noexcept
{
    if (capacity == 0)
    {
        return;
    }

    std::size_t out = 0;

    auto append_char = [&](char c)
    {
        if (out + 1 < capacity)
        {
            dst[out++] = c;
        }
    };

    for (char c : src)
    {
        if (out + 1 >= capacity)
        {
            break;
        }

        switch (c)
        {
            case '\\':
                append_char('\\');
                append_char('\\');
                break;

            case '\n':
                append_char('\\');
                append_char('n');
                break;

            case '\r':
                append_char('\\');
                append_char('r');
                break;

            case '\t':
                append_char('\\');
                append_char('t');
                break;

            default:
                append_char(c);
                break;
        }
    }

    dst[out] = '\0';
}

inline std::string gdb_keep_to_string(const std::string& value)
{
    return value;
}

inline std::string gdb_keep_to_string(std::string_view value)
{
    return std::string(value);
}

inline std::string gdb_keep_to_string(const char* value)
{
    return value ? std::string(value) : std::string("<null>");
}

inline std::string gdb_keep_to_string(char* value)
{
    return value ? std::string(value) : std::string("<null>");
}

template <class T>
inline std::string gdb_keep_to_string(const T& value)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        return std::to_string(value);
    }
    else if constexpr (
        requires(std::ostringstream& os, const T& v)
        {
            os << v;
        })
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
    else
    {
        return "<unsupported type>";
    }
}

inline GdbKeepEntry* gdb_find_or_allocate_entry(
    const char* name,
    const char* file) noexcept
{
    GdbKeepEntry* free_entry = nullptr;

    for (auto& entry : g_gdb_keep_registry.entries)
    {
        if (!entry.active)
        {
            if (!free_entry)
            {
                free_entry = &entry;
            }
            continue;
        }

        // Repeated KEEP_FOR_GDB(var) calls in the same invocation update the
        // previous snapshot instead of consuming another slot.
        if (std::string_view(entry.name) == name &&
            std::string_view(entry.file) == file)
        {
            return &entry;
        }
    }

    return free_entry;
}

template <class T>
GDB_DIAGNOSTIC_FUNCTION
void gdb_keep_for_core(
    const char* name,
    const T& value,
    const char* file,
    std::uint32_t line)
{
    GdbKeepEntry* entry = gdb_find_or_allocate_entry(name, file);
    if (!entry)
    {
        return;
    }

    const std::string value_string = gdb_keep_to_string(value);

    gdb_copy_text(entry->name, sizeof(entry->name), name);
    gdb_copy_text(entry->file, sizeof(entry->file), file);
    gdb_copy_escaped_value(
        entry->value,
        sizeof(entry->value),
        value_string);

    entry->line = line;
    entry->active = 1;

    // Publish a normal global pointer to this thread's TLS registry. GDB can
    // dereference this from a core dump without performing TLS resolution.
    gdb_publish_keep_registry_for_core();

    // Force the completed snapshot to be materialized before execution can
    // continue toward a possible crash.
    asm volatile("" : : "m"(*entry), "m"(g_gdb_keep_registry) : "memory");
}

class GdbKeepFrameGuard
{
public:
    explicit GdbKeepFrameGuard(const void* frame) noexcept
        : m_frame(reinterpret_cast<std::uintptr_t>(frame))
    {
        auto& registry = g_gdb_keep_registry;

        // A new generation starts when:
        //   1. there is no active invocation anymore, OR
        //   2. KEEP_FOR_GDB is reached from another physical stack frame.
        //
        // Case (1) is what makes repeated calls to the same function safe
        // even when the exact same stack address is reused.
        if (!registry.invocation_active ||
            registry.owner_frame != m_frame)
        {
            registry.begin_new_invocation(frame);
        }

        ++registry.active_guards;
        m_generation = registry.generation;

        asm volatile("" : : "m"(registry) : "memory");
    }

    ~GdbKeepFrameGuard() noexcept
    {
        auto& registry = g_gdb_keep_registry;

        // A nested diagnostic function may have replaced the registry owner
        // and generation while this guard was alive. Never decrement another
        // invocation's guard count.
        if (!registry.invocation_active ||
            registry.owner_frame != m_frame ||
            registry.generation != m_generation)
        {
            return;
        }

        if (registry.active_guards > 0)
        {
            --registry.active_guards;
        }

        if (registry.active_guards == 0)
        {
            // Do NOT clear owner_frame, generation, or entries here.
            // If destruction happens during exception unwinding and the
            // program aborts later in unhandled_exception(), the last captured
            // values must still be present in the core dump.
            registry.invocation_active = 0;
        }

        asm volatile("" : : "m"(registry) : "memory");
    }

    GdbKeepFrameGuard(const GdbKeepFrameGuard&) = delete;
    GdbKeepFrameGuard& operator=(const GdbKeepFrameGuard&) = delete;
    GdbKeepFrameGuard(GdbKeepFrameGuard&&) = delete;
    GdbKeepFrameGuard& operator=(GdbKeepFrameGuard&&) = delete;

private:
    std::uintptr_t m_frame = 0;
    std::uint64_t m_generation = 0;
};

#define GDB_KEEP_CONCAT_INNER(a, b) a##b
#define GDB_KEEP_CONCAT(a, b) GDB_KEEP_CONCAT_INNER(a, b)

#define GDB_KEEP_IMPL(var, id)                                                \
    [[maybe_unused]] GdbKeepFrameGuard GDB_KEEP_CONCAT(                       \
        __gdb_keep_guard_, id)(__builtin_frame_address(0));                   \
    gdb_keep_for_core(                                                        \
        #var,                                                                 \
        (var),                                                                \
        __FILE__,                                                             \
        static_cast<std::uint32_t>(__LINE__));                                \
    asm volatile("" : : "g"(&(var)), "m"(g_gdb_keep_registry) : "memory")

// KEEP_FOR_GDB intentionally expands to a declaration + statements rather than
// a do { ... } while(false) block. The guard must live until the end of the
// enclosing C++ scope; otherwise it would become inactive immediately after the
// macro call and the next KEEP_FOR_GDB in the same function would incorrectly
// start another generation.
#define KEEP_FOR_GDB(var) \
    GDB_KEEP_IMPL(var, __COUNTER__)
