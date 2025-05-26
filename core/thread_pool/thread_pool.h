#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <queue>

#include "thread_worker.h"
#include <util_macros.h>

class ThreadPool
{
private:
    std::vector<std::thread> m_thread_list;
    std::queue<std::function<void()>> m_function_queue;
    std::mutex m_queue_mutex;

public:
    bool m_is_shut_down = false;
    bool m_is_write_log = true;
    int m_total_threads = 0;
    int m_running_threads = 0;
    std::string m_thread_pool_name;
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;

    ThreadPool(int n_theards, const std::string& name);
    ~ThreadPool();

    std::function<void()> get_function();
    bool is_function_queue_empty();

    void execute_function(std::function<void()> request_handler);
    void update_counting_running_thread(int amount);
    void set_write_log(bool val);
    void shut_down();
};

