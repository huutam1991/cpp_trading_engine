#pragma once

#include <atomic>

class SpinLock 
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    void lock() 
    {
        while (flag.test_and_set(std::memory_order_acquire));
    }
    
    void unlock() 
    {
        flag.clear(std::memory_order_release);
    }
};

class SpinLockGuard 
{
    SpinLock& lock_;

public:
    explicit SpinLockGuard(SpinLock& l) : lock_(l) { lock_.lock(); }
    ~SpinLockGuard() { lock_.unlock(); }
};