#pragma once

#include <cstddef>
#include <system_io/system_io_object.h>

struct BasePromiseType;

struct TaskInfoEvent : public NamedIOObject<TaskInfoEvent>
{
    enum TaskType
    {
        NONE,
        RUN,
        SET_SUSPEND_VALUE,
        REMOVE_AWAITER
    };

    TaskType type;
    BasePromiseType* promise;

    TaskInfoEvent() : type(TaskType::NONE), promise(nullptr) {};
    TaskInfoEvent(std::nullptr_t) : type(TaskType::NONE), promise(nullptr) {}
    TaskInfoEvent(TaskType type, BasePromiseType* promise) : type(type), promise(promise) {}

    bool operator==(std::nullptr_t) const
    {
        return type == TaskType::NONE && promise == nullptr;
    }

    void check_handle();

    // SystemIOObject's methods
    virtual int generate_fd() override;
    virtual int get_io_events() { return EPOLLIN; }
    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;
};