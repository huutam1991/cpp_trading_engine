#pragma once

#include <cstddef>

struct BasePromiseType;

struct TaskInfoEvent
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
};