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

    virtual bool is_json_value() = 0;
    virtual void write_string_value(JsonStringBuilder& builder) = 0;
    virtual JsonTypeBase* get_copy() = 0;
    virtual JsonTypeBase* get_deep_clone() = 0;
    virtual void release() = 0;
};
