#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <vector>
#include <algorithm>

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

struct CoreDumpInfo
{
    fs::path path;
    fs::file_time_type last_write_time;
    uintmax_t size_bytes = 0;
};

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
            // TODO:
            // insert_crash_log_to_mongodb({
            //     env_name,
            //     core.path.string(),
            //     core.size_bytes,
            //     backtrace
            // });

            MongoDB::instance()
                .set_db_and_collection("system_monitoring", "crash_log")
                .insert_one(Json{
                    {"env", env_name},
                    {"core_file", core.path.string()},
                    {"size_bytes", core.size_bytes},
                    {"backtrace", backtrace}
            });

            spdlog::info("Generated backtrace size={} bytes for {}",
                         backtrace.size(),
                         core.path.string());
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