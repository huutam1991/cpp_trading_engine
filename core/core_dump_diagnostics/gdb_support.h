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
    // Physical stack frame that owns the current diagnostic snapshot.
    // KEEP_FOR_GDB obtains this from __builtin_frame_address(0) at the call
    // site, before entering any helper function.
    std::uintptr_t owner_frame = 0;

    // Incremented every time a different stack frame becomes the owner.
    // Besides being useful in a core dump, this makes every automatic reset
    // observable and gives us a simple generation number for the snapshot.
    std::uint64_t generation = 0;

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

    void reset_for_frame(const void* frame) noexcept
    {
        owner_frame = reinterpret_cast<std::uintptr_t>(frame);
        ++generation;
        clear_entries();
    }
};

// One registry per OS thread. Values are copied into fixed POD buffers so GDB
// can read them directly from a core dump without depending on coroutine-local
// DWARF layout or libstdc++ pretty printers.
inline thread_local GdbKeepRegistry g_gdb_keep_registry __attribute__((used));

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

inline void gdb_prepare_keep_registry(const void* current_frame) noexcept
{
    const auto frame = reinterpret_cast<std::uintptr_t>(current_frame);

    if (g_gdb_keep_registry.owner_frame == frame)
    {
        return;
    }

    g_gdb_keep_registry.reset_for_frame(current_frame);

    // Make the ownership/reset visible before KEEP_FOR_GDB writes the first
    // variable into this generation.
    asm volatile("" : : "m"(g_gdb_keep_registry) : "memory");
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

        // Repeated KEEP_FOR_GDB(var) calls in the same owning frame update the
        // previous snapshot rather than consuming another slot.
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
inline void gdb_keep_for_core(
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

    // Ensure the completed snapshot is materialized before execution proceeds
    // toward a possible crash.
    asm volatile("" : : "m"(*entry) : "memory");
}

// No explicit RESET_KEEP_FOR_GDB() is required.
//
// The first KEEP_FOR_GDB() reached from a different physical stack frame
// automatically starts a new registry generation and clears stale entries.
// Consecutive KEEP_FOR_GDB() calls from the same frame accumulate variables.
//
// Important for coroutines: if the coroutine suspends and later resumes on a
// different physical stack frame, that resume is intentionally treated as a
// new generation.
#define KEEP_FOR_GDB(var)                                                     \
    do                                                                        \
    {                                                                         \
        const void* const __gdb_keep_frame = __builtin_frame_address(0);      \
        gdb_prepare_keep_registry(__gdb_keep_frame);                          \
        gdb_keep_for_core(                                                    \
            #var,                                                             \
            (var),                                                            \
            __FILE__,                                                         \
            static_cast<std::uint32_t>(__LINE__));                            \
        asm volatile("" : : "g"(&(var)), "m"(g_gdb_keep_registry) : "memory"); \
    } while (false)
