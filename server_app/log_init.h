#pragma once

#include <spdlog/async.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

struct LogInit
{
    static void init()
    {
        std::string log_level = std::getenv("LOG_LEVEL") ? std::getenv("LOG_LEVEL") : "trace";
        auto log_level_enum = spdlog::level::from_str(log_level);
        auto async_logger = spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>("async_logger");
        async_logger->set_pattern("%d-%m-%Y %H:%M:%S %^%l%$ %v");
        async_logger->set_level(log_level_enum);
        spdlog::set_default_logger(async_logger);
        spdlog::info("Logging initialized with level: {}", log_level); 
    }
};