#pragma once

#include <future>
#include "task_void.h"
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
    };

    Task(promise_type* promise) : BaseTask<T>(promise) {}
    Task() = default;
    Task(const Task&) = delete;
    Task(Task&&) = default;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = default;
};