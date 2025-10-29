#pragma once

#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <spdlog/spdlog.h>

struct ThreadPinning
{
    static void pin_thread_to_core(int core_id)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);

        pthread_t current_thread = pthread_self();

        int result = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
        if (result != 0)
        {
            spdlog::error("Failed to set thread affinity: {}", strerror(result));
        }
        else
        {
            spdlog::info("Pinned thread {} to core {}", syscall(SYS_gettid), core_id);
        }
    }
};