#ifndef THREAD_WORKER_H
#define THREAD_WORKER_H

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

#endif // THREAD_WORKER_H
