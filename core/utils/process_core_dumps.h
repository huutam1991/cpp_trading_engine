#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <vector>
#include <algorithm>
#include <regex>
#include <sstream>
#include <vector>
#include <string>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>

#include <spdlog/spdlog.h>
#include <json/json.h>
#include <mongo_db/mongo_db.h>

using bsoncxx::builder::basic::kvp;

namespace fs = std::filesystem;

struct CoreDumpInfo
{
    fs::path path;
    fs::file_time_type last_write_time;
    uintmax_t size_bytes = 0;
};

struct StackFrameInfo
{
    int index = -1;
    std::string function;
    std::string file;
    std::string line;
    std::string raw;
};

static bool is_noise_function(const std::string& fn)
{
    return fn.find("std::") == 0 ||
           fn.find("__GI_") != std::string::npos ||
           fn.find("start_thread") != std::string::npos ||
           fn.find("clone") != std::string::npos ||
           fn.find("pthread") != std::string::npos ||
           fn.find("epoll_wait") != std::string::npos ||
           fn.find("poll") != std::string::npos;
}

static std::vector<StackFrameInfo> parse_stack_frames(const std::string& backtrace)
{
    std::vector<StackFrameInfo> frames;
    std::istringstream iss(backtrace);
    std::string line;

    std::regex frame_regex(
        R"(^#([0-9]+)\s+(?:0x[0-9a-fA-F]+\s+in\s+)?(.+?)(?:\s+at\s+(.+):([0-9]+))?$)"
    );

    while (std::getline(iss, line))
    {
        std::smatch match;

        if (!std::regex_match(line, match, frame_regex))
        {
            continue;
        }

        StackFrameInfo frame;
        frame.index = std::stoi(match[1].str());
        frame.function = match[2].str();
        frame.file = match[3].matched ? match[3].str() : "";
        frame.line = match[4].matched ? match[4].str() : "";
        frame.raw = line;

        frames.push_back(std::move(frame));
    }

    return frames;
}


static std::string extract_current_thread_backtrace(const std::string& backtrace)
{
    const std::string marker = "[Current thread is ";

    size_t start = backtrace.find(marker);
    if (start == std::string::npos)
    {
        return backtrace;
    }

    size_t next_thread = backtrace.find("\nThread ", start + marker.size());
    if (next_thread == std::string::npos)
    {
        return backtrace.substr(start);
    }

    return backtrace.substr(start, next_thread - start);
}

static bool is_project_frame(const StackFrameInfo& frame)
{
    return !frame.file.empty() && frame.file.find("cpp_trading_engine/") != std::string::npos;
}

Json parse_crash_backtrace_to_json(const std::string& backtrace)
{
    Json result;

    std::string signal = "UNKNOWN";

    {
        std::regex signal_regex(R"(Program terminated with signal ([A-Z0-9]+))");
        std::smatch match;

        if (std::regex_search(backtrace, match, signal_regex))
        {
            signal = match[1].str();
        }
    }

    std::string crash_thread_backtrace = extract_current_thread_backtrace(backtrace);
    auto frames = parse_stack_frames(crash_thread_backtrace);

    result["signal"] = signal;
    result["frame_count"] = frames.size();

    if (!frames.empty())
    {
        result["crash_frame_index"] = frames[0].index;
        result["crash_function"] = frames[0].function;
        result["crash_file"] = frames[0].file;
        result["crash_line"] = frames[0].line;
    }

    StackFrameInfo* suspect = nullptr;

    for (auto& frame : frames)
    {
        if (is_project_frame(frame) && !is_noise_function(frame.function))
        {
            suspect = &frame;
            break;
        }
    }

    if (suspect)
    {
        result["suspect_frame_index"] = suspect->index;
        result["suspect_function"] = suspect->function;
        result["suspect_file"] = suspect->file;
        result["suspect_line"] = suspect->line;
    }

    Json call_path;
    size_t call_path_index = 0;

    for (const auto& frame : frames)
    {
        if (!is_project_frame(frame))
        {
            continue;
        }

        if (is_noise_function(frame.function))
        {
            continue;
        }

        Json item;

        item["frame_index"] = frame.index;
        item["function"] = frame.function;
        item["file"] = frame.file;
        item["line"] = frame.line;

        call_path[call_path_index++] = std::move(item);
    }

    result["call_path"] = std::move(call_path);

    return result;
}

void insert_crash_log_to_mongodb(
    mongocxx::collection& collection,
    const std::string& env_name,
    const std::string& core_file,
    int64_t core_size_bytes,
    const std::string& backtrace)
{
    try
    {
        Json crash_info = parse_crash_backtrace_to_json(backtrace);

        bsoncxx::builder::basic::document doc;

        doc.append(
            kvp("app", "cpp_trading_engine"),
            kvp("env", env_name),
            kvp("core_file", core_file),
            kvp("core_file_size_bytes", core_size_bytes),
            kvp("backtrace_size", static_cast<int64_t>(backtrace.size())),

            kvp("signal", (std::string)crash_info["signal"]),
            kvp("frame_count", static_cast<int64_t>((size_t)crash_info["frame_count"])),

            kvp("crash_function", (std::string)crash_info["crash_function"]),
            kvp("crash_file", (std::string)crash_info["crash_file"]),
            kvp("crash_line", (std::string)crash_info["crash_line"]),

            kvp("suspect_function", (std::string)crash_info["suspect_function"]),
            kvp("suspect_file", (std::string)crash_info["suspect_file"]),
            kvp("suspect_line", (std::string)crash_info["suspect_line"]),

            kvp("created_at", bsoncxx::types::b_date{
                std::chrono::system_clock::now()
            }),
            kvp("created_at_ns", static_cast<int64_t>(
                Utils::get_time_now_in_utc_nanoseconds()
            ))
        );

        auto array_builder = bsoncxx::builder::basic::array{};

        crash_info["call_path"].for_each([&array_builder](Json& item)
        {
            bsoncxx::builder::basic::document frame_doc;

            frame_doc.append(
                kvp("frame_index", static_cast<int64_t>((size_t)item["frame_index"])),
                kvp("function", (std::string)item["function"]),
                kvp("file", (std::string)item["file"]),
                kvp("line", (std::string)item["line"])
            );

            array_builder.append(frame_doc.extract());
        });

        doc.append(kvp("call_path", array_builder.extract()));

        collection.insert_one(doc.view());

        spdlog::info("Inserted parsed crash log to MongoDB for {}", core_file);
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to insert parsed crash log to MongoDB: {}", e.what());
    }
}

static std::string run_command_capture_output(const std::string& cmd)
{
    std::array<char, 4096> buffer{};
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }

    pclose(pipe);
    return result;
}

static std::vector<CoreDumpInfo> find_core_files()
{
    std::vector<CoreDumpInfo> cores;

    constexpr std::string_view prefix = "core.http_server_cpp.";

    for (const auto& entry : fs::directory_iterator("/tmp"))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const auto filename = entry.path().filename().string();

        if (!filename.starts_with(prefix))
        {
            continue;
        }

        std::error_code ec;

        CoreDumpInfo info;
        info.path = entry.path();
        info.last_write_time = fs::last_write_time(entry.path(), ec);
        info.size_bytes = fs::file_size(entry.path(), ec);

        cores.push_back(std::move(info));
    }

    std::sort(cores.begin(), cores.end(),
              [](const CoreDumpInfo& a, const CoreDumpInfo& b)
              {
                  return a.last_write_time < b.last_write_time;
              });

    return cores;
}

static std::string shell_quote(const std::string& s)
{
    std::string out = "'";
    for (char c : s)
    {
        if (c == '\'')
        {
            out += "'\\''";
        }
        else
        {
            out += c;
        }
    }
    out += "'";
    return out;
}

static std::string generate_backtrace_from_core(
    const std::string& binary_path,
    const fs::path& core_file)
{
    std::string cmd =
        "gdb " + shell_quote(binary_path) + " " + shell_quote(core_file.string()) +
        " -batch "
        "-ex 'set pagination off' "
        "-ex 'bt full' "
        "-ex 'thread apply all bt full' 2>&1";

    return run_command_capture_output(cmd);
}

void process_old_core_dumps_on_startup(
    const std::string& binary_path,
    const std::string& env_name)
{
    auto cores = find_core_files();

    if (cores.empty())
    {
        spdlog::info("No old core dump files found");
        return;
    }

    spdlog::warn("Found {} old core dump file(s)", cores.size());

    for (const auto& core : cores)
    {
        spdlog::warn("Processing old core dump: {}", core.path.string());

        std::string backtrace = generate_backtrace_from_core(binary_path, core.path);

        if (backtrace.empty())
        {
            spdlog::error("Failed to generate backtrace for {}", core.path.string());
        }
        else
        {
            GET_COLLECTION("system_monitoring", "crash_log", collection);
            insert_crash_log_to_mongodb(collection,
                                        env_name,
                                        core.path.string(),
                                        core.size_bytes,
                                        backtrace);

            spdlog::info("Generated backtrace size={} bytes for {}", backtrace.size(), core.path.string());
        }

        std::error_code ec;
        fs::remove(core.path, ec);

        if (ec)
        {
            spdlog::error("Failed to remove core file {}: {}",
                          core.path.string(),
                          ec.message());
        }
        else
        {
            spdlog::info("Removed old core file: {}", core.path.string());
        }
    }
}