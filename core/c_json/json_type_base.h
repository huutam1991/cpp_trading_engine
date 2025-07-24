#pragma once

#include <string>
#include <memory>

class JsonTypeBaseNew
{
protected:
    JsonTypeBaseNew(const JsonTypeBaseNew&) = delete;
    JsonTypeBaseNew() = default;

public:
    virtual ~JsonTypeBaseNew() {};

    virtual bool is_json_value() = 0;
    virtual const std::string get_string_value() const = 0;
    virtual JsonTypeBaseNew* get_copy() = 0;
    virtual void release() = 0;
};
