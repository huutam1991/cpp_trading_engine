#pragma once

#include <variant>
#include <system_io/system_io_object.h>

#include "event_base.h"

class EpollBase : public EventBase
{
    int m_epoll_fd;
    int m_shutdown_fd;

    void add_fd(int fd, SystemIOObject* ptr);
    int create_shutdown_event();
    void set_ready_task(SystemIOObject* object);

    struct TaskInfoEventEpoll : public TaskInfoEvent, NamedIOObject<TaskInfoEvent>
    {
        // SystemIOObject's methods
        virtual int generate_fd() override;
        virtual int get_io_events() { return EPOLLIN; }
        virtual int activate() override;
        virtual int handle_read() override;
        virtual int handle_write() override;
        virtual void release() override;
    };

    using TaskInfoEventPool = CachePool<TaskInfoEventEpoll, MAX_TASK_INFO>;

public:
    EpollBase(size_t id);
    virtual ~EpollBase() override;

    void mod_fd_events(int fd, SystemIOObject* ptr, uint32_t events);
    void del_fd(int fd, SystemIOObject* ptr);
    void start_living_system_io_object(SystemIOObject* object);

    virtual inline void add_run_task_event(BasePromiseType* promise) override
    {
        TaskInfoEventEpoll* task_event = TaskInfoEventPool::acquire();
        task_event->type = TaskInfoEvent::TaskType::RUN;
        task_event->promise = promise;

        set_ready_task(task_event);
    }

    virtual inline void add_set_suspend_value_event(BasePromiseType* promise) override
    {
        TaskInfoEventEpoll* task_event = TaskInfoEventPool::acquire();
        task_event->type = TaskInfoEvent::TaskType::SET_SUSPEND_VALUE;
        task_event->promise = promise;

        set_ready_task(task_event);
    }

    virtual inline void add_remove_awaiter_event(BasePromiseType* promise) override
    {
        TaskInfoEventEpoll* task_event = TaskInfoEventPool::acquire();
        task_event->type = TaskInfoEvent::TaskType::REMOVE_AWAITER;
        task_event->promise = promise;

        set_ready_task(task_event);
    }

    virtual inline void add_force_destroy_event(BasePromiseType* promise) override
    {
        TaskInfoEventEpoll* task_event = TaskInfoEventPool::acquire();
        task_event->type = TaskInfoEvent::TaskType::FORCE_DESTROY;
        task_event->promise = promise;

        set_ready_task(task_event);
    }

    virtual void stop() override;
    virtual void loop() override;
};