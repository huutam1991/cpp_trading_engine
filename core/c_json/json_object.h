#pragma once

#include <c_json/json_type_base.h>
#include <cache/cache_pool.h>

class JsonObject;
using JsonValuePool = CachePool<JsonObject, 10000>;

class JsonObject : public JsonTypeBase
{
    uint32_t reference_count = 0; // Reference count for shared ownership

public:
    JsonObject() = default;
    JsonObject(const JsonObject&) = delete;
    JsonObject(JsonObject&&) = delete;
    JsonObject& operator=(const JsonObject&) = delete;
    JsonObject& operator=(JsonObject&&) = delete;

    virtual ~JsonObject() override = default;

    virtual bool is_json_value() override
    {
        return false; // This is not a JSON value, but an object
    }

    virtual const std::string get_string_value() const override
    {
        return "{}"; // Placeholder for object representation
    }

    virtual JsonTypeBase* get_copy() override
    {
        reference_count++;
        return this;
    }
};