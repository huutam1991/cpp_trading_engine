#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#define GDB_DIAGNOSTIC_FUNCTION \
    __attribute__((noinline, optimize("O0")))

inline constexpr std::size_t GDB_KEEP_MAX_VARIABLES = 16;
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
    std::array<GdbKeepEntry, GDB_KEEP_MAX_VARIABLES> entries{};

    void clear() noexcept
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
};

// One registry per OS thread. Because the values themselves are copied into POD
// char buffers, GDB can read them directly from a core dump without relying on
// coroutine-local DWARF or libstdc++ pretty printers.
inline thread_local GdbKeepRegistry g_gdb_keep_registry __attribute__((used));

inline void gdb_copy_text(char* dst, std::size_t capacity, std::string_view src) noexcept
{
    if (capacity == 0)
    {
        return;
    }

    const std::size_t n = src.size() < capacity - 1 ? src.size() : capacity - 1;
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
    else if constexpr (requires(std::ostringstream& os, const T& v) { os << v; })
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
    gdb_copy_escaped_value(entry->value, sizeof(entry->value), value_string);
    entry->line = line;
    entry->active = 1;

    // Keep the registry write visible to the compiler and materialized before
    // execution proceeds toward a possible crash.
    asm volatile("" : : "m"(*entry) : "memory");
}

// Explicitly clear stale values before starting the flow being investigated.
// Do NOT make this RAII cleanup: if an exception unwinds into a coroutine
// unhandled_exception() that later aborts, a destructor-based cleanup could erase
// the evidence before the core dump is produced.
#define RESET_KEEP_FOR_GDB() \
    do \
    { \
        g_gdb_keep_registry.clear(); \
        asm volatile("" : : "m"(g_gdb_keep_registry) : "memory"); \
    } while (false)

// Snapshot the current value into the crashing thread's TLS registry.
// Calling it again for the same variable updates the existing entry.
#define KEEP_FOR_GDB(var) \
    do \
    { \
        gdb_keep_for_core(#var, (var), __FILE__, static_cast<std::uint32_t>(__LINE__)); \
        asm volatile("" : : "g"(&(var)) : "memory"); \
    } while (false)
