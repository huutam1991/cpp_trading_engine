#pragma once

#include <string>
#include <memory>

#include <json/json_string_builder.h>

class JsonTypeBase
{
protected:
    JsonTypeBase(const JsonTypeBase&) = delete;
    JsonTypeBase() = default;

public:
    virtual ~JsonTypeBase() {};

protected:
    using IsJsonValuePtr = bool (JsonTypeBase::*)();
    IsJsonValuePtr is_json_value_ptr = nullptr;

    using GetCopyPtr = JsonTypeBase* (JsonTypeBase::*)();
    GetCopyPtr get_copy_ptr = nullptr;
    GetCopyPtr get_deep_clone_ptr = nullptr;

    using WriteStringValuePtr = void (JsonTypeBase::*)(JsonStringBuilder& builder);
    WriteStringValuePtr write_string_value_ptr = nullptr;

    using ReleasePtr = void (JsonTypeBase::*)();
    ReleasePtr release_ptr = nullptr;

public:
    inline bool is_json_value()
    {
        return (this->*is_json_value_ptr)();
    }

    inline JsonTypeBase* get_copy()
    {
        return (this->*get_copy_ptr)();
    }

    inline JsonTypeBase* get_deep_clone()
    {
        return (this->*get_deep_clone_ptr)();
    }

    inline void write_string_value(JsonStringBuilder& builder)
    {
        (this->*write_string_value_ptr)(builder);
    }

    inline void release()
    {
        (this->*release_ptr)();
    }
};
