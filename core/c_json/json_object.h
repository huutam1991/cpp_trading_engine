#pragma once

#include <unordered_map>
#include <vector>

#include <c_json/json_type_base.h>
#include <c_json/json.h>
#include <cache/cache_pool.h>

class JsonObject;
using JsonObjectPool = CachePool<JsonObject, 10000>;

class JsonObject : public JsonTypeBaseNew
{
    uint32_t reference_count = 0; // Reference count for shared ownership
    bool m_is_array = false; // Flag to indicate if this is an array
    std::unordered_map<std::string, JsonNew> m_object; // Key-value pairs for JSON object
    std::vector<JsonNew> m_array; // Array for JSON object

public:
    JsonObject() = default;
    JsonObject(const JsonObject&) = delete;
    JsonObject(JsonObject&&) = delete;
    JsonObject& operator=(const JsonObject&) = delete;
    JsonObject& operator=(JsonObject&&) = delete;

    virtual ~JsonObject() override = default;

    JsonNew& operator[](const char* key)
    {
        return m_object[key];
    }

    JsonNew& operator[](size_t index)
    {
        if (index >= m_array.size())
        {
            m_array.resize(index + 1);
        }

        return m_array[index];
    }

    // Methosds from JsonTypeBaseNew
    virtual void init() override
    {
        reference_count = 1; // Initialize reference count to 1
    }

    virtual bool is_json_value() override
    {
        return false; // This is not a JSON value, but an object
    }

    virtual const std::string get_string_value() const override
    {
        return "{}"; // Placeholder for object representation
    }

    virtual JsonTypeBaseNew* get_copy() override
    {
        reference_count++;
        return this;
    }

    virtual void release() override
    {
        spdlog::debug("JsonObject release called, current reference count: {}, this: {}", reference_count, (size_t)this);
        std::cout << "check release 1" << std::endl;
        reference_count--;
        std::cout << "check release 2" << std::endl;
        if (reference_count == 0)
        {
        std::cout << "check release 3" << std::endl;
            JsonObjectPool::release(this);
        std::cout << "check release 4" << std::endl;
        }
        std::cout << "check release 5" << std::endl;
    }
};