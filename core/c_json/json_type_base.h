#pragma once

#include <string>
#include <memory>

#include <c_json/json_string_builder.h>

class JsonTypeBaseNew
{
protected:
    JsonTypeBaseNew(const JsonTypeBaseNew&) = delete;
    JsonTypeBaseNew() = default;

public:
    virtual ~JsonTypeBaseNew() {};

    virtual bool is_json_value() = 0;
    virtual void write_string_value(JsonStringBuilder& builder) const = 0;
    virtual JsonTypeBaseNew* get_copy() = 0;
    virtual void release() = 0;
};
