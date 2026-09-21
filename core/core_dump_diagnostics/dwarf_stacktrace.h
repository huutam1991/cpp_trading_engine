#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include <backtrace.h>
#include <cxxabi.h>

struct DwarfStackTrace
{
    static constexpr std::size_t MaxFrames = 64;

    std::array<uintptr_t, MaxFrames> pcs{};
    std::size_t size = 0;

    static DwarfStackTrace capture(std::size_t skip = 0) noexcept
    {
        DwarfStackTrace trace;

        CaptureContext ctx{
            .trace = &trace,
            .skip = skip
        };

        backtrace_simple(state(), 0, capture_callback, error_callback, &ctx);

        return trace;
    }

    void print(std::ostream& out = std::cerr) const
    {
        out << "DWARF stacktrace:\n";

        for (std::size_t i = 0; i < size; ++i)
        {
            PrintContext ctx{
                .out = &out,
                .physical_frame = i,
                .inline_frame = 0
            };

            const int result = backtrace_pcinfo(state(), pcs[i], print_callback, error_callback, &ctx);

            // No DWARF information -> at least print PC.
            if (result != 0 || ctx.inline_frame == 0)
            {
                out << "  #" << i << " 0x" << std::hex << pcs[i] << std::dec << '\n';
            }
        }
    }

private:
    struct CaptureContext
    {
        DwarfStackTrace* trace;
        std::size_t skip;
    };

    struct PrintContext
    {
        std::ostream* out;
        std::size_t physical_frame;
        std::size_t inline_frame;
    };

    static backtrace_state* state() noexcept
    {
        // threaded = 1 because your EventBase/tasks can run from
        // multiple threads.
        static backtrace_state* instance = backtrace_create_state(nullptr, 1, error_callback, nullptr);

        return instance;
    }

    static int capture_callback( void* data, uintptr_t pc) noexcept
    {
        auto& ctx = *static_cast<CaptureContext*>(data);

        if (ctx.skip > 0)
        {
            --ctx.skip;
            return 0;
        }

        if (ctx.trace->size >= MaxFrames)
        {
            return 1;
        }

        ctx.trace->pcs[ctx.trace->size++] = pc;

        return 0;
    }

    static int print_callback(void* data, uintptr_t pc, const char* filename, int line, const char* function) noexcept
    {
        auto& ctx = *static_cast<PrintContext*>(data);

        char* demangled = nullptr;
        if (function != nullptr)
        {
            int status = 0;
            demangled = abi::__cxa_demangle(function, nullptr, nullptr, &status);

            if (status != 0)
            {
                demangled = nullptr;
            }
        }

        const char* name = demangled != nullptr ? demangled : (function != nullptr ? function : "???");

        auto& out = *ctx.out;
        out << "  #" << ctx.physical_frame;

        if (ctx.inline_frame > 0)
        {
            out << "." << ctx.inline_frame;
        }

        out << " " << name;

        if (filename != nullptr)
        {
            out << " at " << filename << ":" << line;
        }

        out << '\n';
        std::free(demangled);
        ++ctx.inline_frame;

        return 0;
    }

    static void error_callback(void*, const char*, int) noexcept
    {
        // Intentionally ignored.
        // Missing DWARF information should not affect application flow.
    }
};