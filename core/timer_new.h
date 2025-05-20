#pragma once

#include <iostream>
#include <functional>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

#include <util_macros.h>

using namespace boost::placeholders;
namespace net = boost::asio;

class TimerNew
{
    Singleton(TimerNew);

public:
    enum TimerUnit
    {
        SECOND = 1000000000,
        MILLISECOND = 1000000,
        MICROSECOND = 1000,
        NANOSECOND = 1,
    };

    void init(boost::asio::io_context& ioc_context);
    void add_schedule_task(std::function<void(void)> callback, size_t tick_interval, TimerUnit unit = TimerUnit::MILLISECOND);

private:
    boost::asio::io_context* m_io_context = nullptr;

    // Class Task (has it's own [m_timer])
    class Task : public std::enable_shared_from_this<Task>
    {
        std::function<void()> m_callback = nullptr;
        std::shared_ptr<boost::asio::steady_timer> m_timer = nullptr;

    public:
        Task(boost::asio::io_context& io_context, std::function<void()> callback, size_t tick_in_nanoseconds);
        void start();
        void on_tick();
    };
};