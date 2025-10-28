#pragma once

#include <cache/cache_pool.h>
#include "system_io_object.h"

struct UserTask : public SystemIOObject
{
    void* task = nullptr;

    inline void clear()
    {
        task = nullptr;
    }

    // SystemIOObject's methods
    virtual int generate_fd();
    virtual int handle_io_data();
    virtual void release();
};

using UserTaskPool = CachePool<UserTask, 10000>;