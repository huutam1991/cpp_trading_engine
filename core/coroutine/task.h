#pragma once

#include <future>
#include "base_promise_type.h"
#include "base_task.h"

template<class T>
struct Task : public BaseTask<T>
{
    struct promise_type : public BaseTask<T>::promise_type
    {
        // Methods of a standard promise
        Task get_return_object()
        {
            return Task{this};
        }

        void return_value(T v)
        {
            this->promise_value.set_value(v);
            this->value = v;
        }

        T value;
    };

    Task(promise_type* promise) : BaseTask<T>(promise) {}
    Task() = default;
    Task(const Task&) = delete;
    Task(Task&&) = default;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = default;

    T await_resume()
    {
        auto& promise = this->handle.promise();
        return ((promise_type*)&promise)->value;
    }
};

template<>
struct Task<void> : public BaseTask<void>
{
    struct promise_type : public BaseTask<void>::promise_type
    {
        // Methods of a standard promise
        Task get_return_object()
        {
            return Task{this};
        }

        void return_void()
        {
            promise_value.set_value();
        }
    };

    Task(promise_type* promise) : BaseTask<void>(promise) {}
    Task() = default;
    Task(const Task&) = delete;
    Task(Task&&) = default;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = default;

    void await_resume()
    {
        return;
    }
};
