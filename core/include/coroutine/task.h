#ifndef TASK_H
#define TASK_H

#include <iostream>
#include <coroutine>

template<class T>
struct Task
{
    struct promise_type
    {
        T value;
        Task get_return_object()
        {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(int v) { value = v; }
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;
    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Task() { if (handle) handle.destroy(); }

    bool await_ready()
    {
        return handle.promise().value != -1;
    }

    void await_suspend(std::coroutine_handle<> waiting_handle)
    {
        handle.resume();

        waiting_handle.resume();
    }

    int await_resume()
    {
        return handle.promise().value;
    }
};

#endif //TASK_H