#pragma once

#include <iostream>
#include <functional>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

#include <coroutine/future.h>
#include <coroutine/epoll_base.h>
#include <coroutine/event_base_manager.h>
#include <system_io/timer_io.h>

using namespace boost::placeholders;
namespace net = boost::asio;

class Timer
{
public:
    enum TimerUnit
    {
        SECOND = 1000000000,
        MILLISECOND = 1000000,
        MICROSECOND = 1000,
        NANOSECOND = 1,
    };

    static EpollBase* get_epoll_base();
    static void add_schedule_task(std::function<void()> callback, size_t tick_interval, TimerUnit unit = TimerUnit::MILLISECOND);
    static Future<size_t> sleep_for(size_t tick_interval, TimerUnit unit = TimerUnit::MILLISECOND);
};