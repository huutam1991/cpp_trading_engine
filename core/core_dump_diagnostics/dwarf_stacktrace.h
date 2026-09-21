#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include <backtrace.h>
#include <cxxabi.h>

struct DwarfFrameInfo
{
    std::string function;
    std::string file;

    explicit operator bool() const noexcept
    {
        return !function.empty() || !file.empty();
    }
};

struct DwarfStackTrace
{
    // v2: add logical-frame lookup by depth.
    static constexpr std::uint32_t Version = 2;
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

        backtrace_simple(
            state(),
            0,
            capture_callback,
            error_callback,
            &ctx
        );

        return trace;
    }

    // deep is 1-based and counts logical DWARF frames,
    // including inline frames.
    //
    // Example:
    //
    // deep = 1 -> DwarfStackTrace::capture(...)
    // deep = 2 -> BaseTask::BaseTask(...)
    // deep = 3 -> Task<T>::Task(...)
    // deep = 4 -> promise_type::get_return_object()
    // deep = 5 -> coroutine function that created the Task
    //
    // file format:
    //
    // server_app/api/app_route.cpp:59
    //
    DwarfFrameInfo get_frame_info(std::size_t deep = 5) const
    {
        if (deep == 0)
        {
            return {};
        }

        FrameLookupContext ctx{
            .target_depth = deep,
            .current_depth = 0,
            .found = false,
            .result = {}
        };

        for (std::size_t i = 0; i < size && !ctx.found; ++i)
        {
            backtrace_pcinfo(
                state(),
                pcs[i],
                frame_lookup_callback,
                error_callback,
                &ctx
            );
        }

        return std::move(ctx.result);
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

            const int result = backtrace_pcinfo(
                state(),
                pcs[i],
                print_callback,
                error_callback,
                &ctx
            );

            // No DWARF information -> at least print PC.
            if (result != 0 || ctx.inline_frame == 0)
            {
                out << "  #" << i
                    << " 0x" << std::hex << pcs[i] << std::dec
                    << '\n';
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

    struct FrameLookupContext
    {
        std::size_t target_depth;
        std::size_t current_depth;
        bool found;
        DwarfFrameInfo result;
    };

    static backtrace_state* state() noexcept
    {
        // threaded = 1 because this can be used concurrently
        // by multiple threads.
        static backtrace_state* instance =
            backtrace_create_state(
                nullptr,
                1,
                error_callback,
                nullptr
            );

        return instance;
    }

    static int capture_callback(
        void* data,
        uintptr_t pc) noexcept
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

    static int frame_lookup_callback(
        void* data,
        uintptr_t,
        const char* filename,
        int line,
        const char* function)
    {
        auto& ctx =
            *static_cast<FrameLookupContext*>(data);

        ++ctx.current_depth;

        if (ctx.current_depth != ctx.target_depth)
        {
            return 0;
        }

        ctx.result.function = demangle(function);
        ctx.result.file = format_file(filename, line);
        ctx.found = true;

        // Stop resolving further inline frames for this PC.
        return 1;
    }

    static int print_callback(
        void* data,
        uintptr_t,
        const char* filename,
        int line,
        const char* function) noexcept
    {
        auto& ctx = *static_cast<PrintContext*>(data);

        char* demangled = nullptr;

        if (function != nullptr)
        {
            int status = 0;

            demangled = abi::__cxa_demangle(
                function,
                nullptr,
                nullptr,
                &status
            );

            if (status != 0)
            {
                demangled = nullptr;
            }
        }

        const char* name =
            demangled != nullptr
                ? demangled
                : (function != nullptr ? function : "???");

        auto& out = *ctx.out;

        out << "  #" << ctx.physical_frame;

        if (ctx.inline_frame > 0)
        {
            out << "." << ctx.inline_frame;
        }

        out << " " << name;

        if (filename != nullptr)
        {
            out << " at "
                << filename
                << ":"
                << line;
        }

        out << '\n';

        std::free(demangled);

        ++ctx.inline_frame;

        return 0;
    }

    static std::string demangle(
        const char* function)
    {
        if (function == nullptr)
        {
            return "???";
        }

        int status = 0;

        char* demangled = abi::__cxa_demangle(
            function,
            nullptr,
            nullptr,
            &status
        );

        if (status != 0 || demangled == nullptr)
        {
            std::free(demangled);
            return function;
        }

        std::string result{demangled};

        std::free(demangled);

        return result;
    }

    static std::string format_file(
        const char* filename,
        int line)
    {
        if (filename == nullptr)
        {
            return "???";
        }

        std::string_view path{filename};

        constexpr std::string_view ProjectRoot =
            "cpp_trading_engine/";

        const std::size_t pos =
            path.find(ProjectRoot);

        if (pos != std::string_view::npos)
        {
            path.remove_prefix(
                pos + ProjectRoot.size()
            );
        }

        std::string result{path};

        if (line > 0)
        {
            result += ':';
            result += std::to_string(line);
        }

        return result;
    }

    static void error_callback(
        void*,
        const char*,
        int) noexcept
    {
        // Missing DWARF information should not
        // affect application flow.
    }
};