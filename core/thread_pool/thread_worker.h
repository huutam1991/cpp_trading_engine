#pragma once

#include "thread_pool.h"

class ThreadPool;

class ThreadWorker
{
private:
    ThreadPool* m_thread_pool = nullptr;

public:
    ThreadWorker(ThreadPool* thread_pool);

    void operator()();
};
