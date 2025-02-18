#ifndef TIMER_H
#define TIMER_H

#include <thread>
#include <iostream>
#include <functional>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

#include <util_macros.h>

using namespace boost::placeholders;

class Timer
{
    // Implement Singleton for Timer
    // Need to keep constructor not to be default (to invoke init() in constructor)
public:
    Timer(Timer const&)         = delete;
    Timer& operator=(Timer const&)    = delete;
    static Timer& instance() {
        static Timer instance;
        return instance;
    }
private:
    Timer();
    ~Timer() = default;

private:
    class Task
    {
    public:
        std::function<void(void)> m_work;
        size_t m_period;
        bool   m_is_one_shot = false;
        size_t m_count = 0;

        Task(std::function<void(void)> w, size_t p, bool is_one_shot = false);
        ~Task();
    };

    std::thread m_thread;
    std::mutex m_list_mutex;
    boost::asio::io_service m_io_service;
    std::shared_ptr<boost::asio::deadline_timer> m_timer;
    std::unordered_map<size_t, std::unique_ptr<Task>> m_taks_list;

    size_t get_new_task_id();
    void on_tick();
    void reschedule_next_tick();

public:
    void init();
    size_t add_schedule_task(std::function<void(void)> work, size_t period);
    size_t add_time_out(std::function<void(void)> work, size_t period);
    void delete_schedule_task(size_t task_id);
};

#endif // TIMER_H