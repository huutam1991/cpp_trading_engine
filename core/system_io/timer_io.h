#pragma once

#include <unistd.h>
#include <sys/timerfd.h>
#include <iostream>
#include <functional>

#include <cache/cache_pool.h>
#include "system_io_object.h"

struct TimerIO : public SystemIOObject
{
    size_t interval_ns;
    std::function<void()> callback = nullptr;

    TimerIO() {};

    void set_callback(size_t interval_ns_value, std::function<void()> callback_value);
    void clear();

    // SystemIOObject's methods
    virtual int generate_fd();
    virtual int handle_io_data();
    virtual void release();
};

using TimerIOPool = CachePool<TimerIO, 1000>;
