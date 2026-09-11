#pragma once

// Prevent the compiler from inlining/optimizing a function that is intentionally
// kept easy to inspect in post-mortem GDB analysis.
#define GDB_DIAGNOSTIC_FUNCTION \
    __attribute__((noinline, optimize("O0")))

#define GDB_DETAIL_CONCAT_INNER(a, b) a##b
#define GDB_DETAIL_CONCAT(a, b) GDB_DETAIL_CONCAT_INNER(a, b)

// Keep a local variable materialized and leave a small DWARF-visible marker
// in the same lexical scope. process_core_dumps.h recognizes the marker name
// and records only variables explicitly passed through KEEP_FOR_GDB(...).
//
// IMPORTANT: use this as a standalone statement in the surrounding scope.
#define KEEP_FOR_GDB(var) \
    KEEP_FOR_GDB_IMPL(var, __COUNTER__)

#define KEEP_FOR_GDB_IMPL(var, id) \
    const char* volatile GDB_DETAIL_CONCAT(__gdb_keep_name_, id) = #var; \
    asm volatile("" \
                 : \
                 : "g"(&(var)), "m"(GDB_DETAIL_CONCAT(__gdb_keep_name_, id)) \
                 : "memory")
