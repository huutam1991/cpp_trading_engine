#pragma once

#include <filesystem>
#include <iostream>

#include <spdlog/spdlog.h>

void cleanup_old_core_files()
{
    namespace fs = std::filesystem;

    constexpr auto prefix = "core.http_server_cpp.";

    for (const auto& entry : fs::directory_iterator("/tmp"))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        auto filename = entry.path().filename().string();

        if (filename.starts_with(prefix))
        {
            std::error_code ec;
            fs::remove(entry.path(), ec);

            if (ec)
            {
                spdlog::error("Failed to remove {}: {}", entry.path().string(), ec.message());
            }
            else
            {
                spdlog::info("Removed old core file: {}", entry.path().string());
            }
        }
    }
}