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

public:
    inline bool is_json_value()
    {
        if (is_json_value_ptr != nullptr)
        {
            return (this->*is_json_value_ptr)();
        }
        return false;
    }

    inline virtual void write_string_value(JsonStringBuilder& builder) = 0;
    inline virtual JsonTypeBase* get_copy() = 0;
    inline virtual JsonTypeBase* get_deep_clone() = 0;
    inline virtual void release() = 0;
};
