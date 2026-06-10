#pragma once

#include <cstddef>
#include <system_io/system_io_object.h>

struct BasePromiseType;

struct TaskInfoEvent
{
    enum TaskType
    {
        NONE,
        RUN,
        SET_SUSPEND_VALUE,
        REMOVE_AWAITER,
        FORCE_DESTROY
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
};