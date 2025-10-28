#pragma once

#include <functional>

#include <cache/cache_pool.h>
#include "system_io_object.h"

struct UserTask : public SystemIOObject
{
    void* task = nullptr;
    std::function<int()> handle_function = nullptr;

    void set_handle_function(std::function<int()> func)
    {
        handle_function = std::move(func);
    }

    inline void clear()
    {
        task = nullptr;
        handle_function = nullptr;
    }

    // SystemIOObject's methods
    virtual int generate_fd();
    virtual int handle_io_data();
    virtual void release();
};

using UserTaskPool = CachePool<UserTask, 10000>;