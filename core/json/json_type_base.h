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

template <typename Derived>
class JsonTypeDerived : public JsonTypeBase
{
public:
    JsonTypeDerived(const JsonTypeDerived&) = delete;
    JsonTypeDerived()
    {
        is_json_value_ptr = static_cast<IsJsonValuePtr>(&Derived::is_json_value_method);
        get_copy_ptr = static_cast<GetCopyPtr>(&Derived::get_copy_method);
        get_deep_clone_ptr = static_cast<GetCopyPtr>(&Derived::get_deep_clone_method);
        write_string_value_ptr = static_cast<WriteStringValuePtr>(&Derived::write_string_value_method);
        release_ptr = static_cast<ReleasePtr>(&Derived::release_method);
    }

    virtual ~JsonTypeDerived() override = default;

    bool is_json_value_method()
    {
        return static_cast<Derived*>(this)->is_json_value();
    }

    JsonTypeBase* get_copy_method()
    {
        return static_cast<Derived*>(this)->get_copy();
    }

    JsonTypeBase* get_deep_clone_method()
    {
        return static_cast<Derived*>(this)->get_deep_clone();
    }

    void write_string_value_method(JsonStringBuilder& builder)
    {
        static_cast<Derived*>(this)->write_string_value(builder);
    }

    void release_method()
    {
        static_cast<Derived*>(this)->release();
    }
};
