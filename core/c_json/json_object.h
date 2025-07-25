#pragma once

#include <unordered_map>
#include <vector>

#include <c_json/json_type_base.h>
#include <c_json/json.h>
#include <cache/cache_pool.h>

class JsonObjectNew;
using JsonObjectPool = CachePool<JsonObjectNew, 100000>;

class JsonObjectNew : public JsonTypeBaseNew
{
    uint32_t reference_count = 0; // Reference count for shared ownership
    bool m_is_array = false; // Flag to indicate if this is an array
    std::unordered_map<std::string, JsonNew> m_object; // Key-value pairs for JSON object
    std::vector<JsonNew> m_array; // Array for JSON object

public:
    JsonObjectNew() = default;
    JsonObjectNew(const JsonObjectNew&) = delete;
    JsonObjectNew(JsonObjectNew&&) = delete;
    JsonObjectNew& operator=(const JsonObjectNew&) = delete;
    JsonObjectNew& operator=(JsonObjectNew&&) = delete;

    virtual ~JsonObjectNew() override = default;

    JsonNew& operator[](const char* key)
    {
        m_is_array = false; // This is an object, not an array
        return m_object[key];
    }

    JsonNew& operator[](size_t index)
    {
        m_is_array = true; // This is an array, not an object

        if (index >= m_array.size())
        {
            m_array.resize(index + 1);
        }
        return m_array[index];
    }

    void init()
    {
        reference_count = 1; // Initialize reference count to 1
    }

    void clear()
    {
        m_object.clear();
        m_array.clear();
        m_is_array = false;
    }

    // Methods from JsonTypeBaseNew
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
        reference_count--;
        if (reference_count == 0)
        {
            m_object.clear();
            m_array.clear();
            JsonObjectPool::release(this);
        }
    }
};