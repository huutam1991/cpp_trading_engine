#pragma once

#include "base_promise_type.h"

template<typename T>
struct TaskValue
{
    enum State
    {
        WAITING,
        READY
    };

    alignas(64) std::atomic<State> state{State::WAITING};
    T value;

    T get()
    {
        while (state.load(std::memory_order_acquire) == State::WAITING)
        {
            _mm_pause();
        }

        return value;
    }

    template<typename U>
    void set_value(U&& v)
    {
        value = std::forward<U>(v);
        state.store(State::READY, std::memory_order_release);
    }
};

template<class T>
struct TaskResult
{
    TaskValue<T>* parent;
    TaskResult(TaskValue<T>* parent) : parent(parent) {}

    TaskResult(const TaskResult& copy) = delete;
    TaskResult& operator=(const TaskResult& copy) = delete;

    TaskResult(TaskResult&& copy) : parent(copy.parent)
    {
        copy.parent = nullptr;
    }

    TaskResult& operator=(TaskResult&& copy)
    {
        parent = copy.parent;
        copy.parent = nullptr;
        return *this;
    }

    T get()
    {
        return parent->get();
    }
};

template<>
struct TaskValue<void>
{
    enum State
    {
        WAITING,
        READY
    };

    alignas(64) std::atomic<State> state{WAITING};

    void get()
    {
        while (state.load(std::memory_order_acquire) == WAITING)
        {
            _mm_pause();
        }
    }

    void set_value()
    {
        state.store(READY, std::memory_order_release);
    }
};


template<>
struct TaskResult<void>
{
    TaskValue<void>* parent;
    TaskResult(TaskValue<void>* parent) : parent(parent) {}

    TaskResult(const TaskResult& copy) = delete;
    TaskResult& operator=(const TaskResult& copy) = delete;

    TaskResult(TaskResult&& copy) : parent(copy.parent)
    {
        copy.parent = nullptr;
    }

    TaskResult& operator=(TaskResult&& copy)
    {
        parent = copy.parent;
        copy.parent = nullptr;
        return *this;
    }

    void get()
    {
        return parent->get();
    }
};

template<class T>
struct BaseTask
{
    struct promise_type : public BasePromiseType
    {
        // Methods of a standard promise
        BaseTask get_return_object()
        {
            return BaseTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept
        {
            if (m_suspending_promise != nullptr)
            {
                m_suspending_promise->set_waiting(false);
            }

            return {};
        }
        void unhandled_exception() { std::terminate(); }

        // Promise value
        TaskValue<T> task_value;
    };

    std::coroutine_handle<promise_type> handle = nullptr;

    BaseTask(std::nullptr_t) : handle(nullptr) {}
    BaseTask(std::coroutine_handle<promise_type> h) : handle(h) {}
    BaseTask(promise_type* promise) : handle(std::coroutine_handle<promise_type>::from_promise(*promise)) {}
    BaseTask() {};
    BaseTask(const BaseTask& copy) = delete;
    BaseTask(BaseTask&& copy) : handle{std::move(copy.handle)} { copy.handle = nullptr; }
    ~BaseTask()
    {
        // Light destroy, lol
        destroy(false);
    }

    bool operator==(std::nullptr_t null) const
    {
        return handle == nullptr;
    }

    BaseTask& operator=(const BaseTask& copy) = delete;

    BaseTask& operator=(BaseTask&& copy)
    {
        if (handle != nullptr)
        {
            destroy();
        }

        handle = std::move(copy.handle);
        copy.handle = nullptr;
        return *this;
    }

    void destroy(bool complete = true)
    {
        // This is just a BaseTask object with nullptr handle, not a really BaseTask that is created by C++
        if (handle == nullptr)
        {
            return;
        }

        auto* promise = get_base_promise_type();

        // Hasn't register on EventBase, just destroy the coroutine frame and return
        if (promise->m_event_base == nullptr)
        {
            handle.destroy();
            handle = nullptr;
            return;
        }

        // Already register, mark this task is already release, then it will be destroy later when it's done
        promise->is_task_release = true;
        handle = nullptr;
    }

    // Get BasePromiseType of current coroutine
    BasePromiseType* get_base_promise_type()
    {
        promise_type& promise = handle.promise();
        return &promise;
    }

    void save_suspending_promise(BasePromiseType* suspend_base_pt)
    {
        get_base_promise_type()->m_suspending_promise = suspend_base_pt;
    }

    void register_on(EventBase* event_base)
    {
        auto base_promise_type = get_base_promise_type();
        base_promise_type->register_on(event_base, handle);
    }

    inline TaskResult<T> start_running_on(EventBase* event_base)
    {
        register_on(event_base);
        return TaskResult<T>{&handle.promise().task_value};
    }

    bool await_ready()
    {
        return handle.done();
    }

    template<class promise_type>
    void await_suspend(std::coroutine_handle<promise_type> suspend_handle)
    {
        // Tricky here, cast promise_type to a pointer of BasePromiseType (suppose all of promise_type is child class of BasePromiseType class)
        promise_type& promise = suspend_handle.promise();
        BasePromiseType *suspend_base_pt = &promise;
        suspend_base_pt->set_waiting(true);

        // Save to suspending_promise
        save_suspending_promise(suspend_base_pt);

        // Running this task on EventBase
        register_on(suspend_base_pt->m_event_base);
    }
};