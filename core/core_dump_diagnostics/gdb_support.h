#pragma once

#define GDB_DIAGNOSTIC_FUNCTION \
    __attribute__((noinline, optimize("O0")))

#define KEEP_FOR_GDB(var) \
    asm volatile("" : : "g"(&(var)) : "memory")