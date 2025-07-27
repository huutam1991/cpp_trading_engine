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

    void add_pair(std::pair<std::string, JsonNew> field)
    {
        m_is_array = false; // This is an object, not an array
        m_object.insert(std::move(field));
    }

    JsonNew& operator[](const char* key)
    {
        m_is_array = false; // This is an object, not an array
        auto [it, inserted] = m_object.emplace(key, JsonNew());
        return it->second;
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

    void for_each(std::function<void(JsonNew&)>& loop_func);
    void for_each_with_key(std::function<void(const std::string&,JsonNew&)>& loop_func);
    void for_each_with_index(std::function<void(size_t,JsonNew&)>& loop_func);

    // For iterating through the object
    using iterator = std::unordered_map<std::string, JsonNew>::iterator;
    using const_iterator = std::unordered_map<std::string, JsonNew>::const_iterator;
    iterator begin() { return m_object.begin(); }
    iterator end() { return m_object.end(); }
    const_iterator begin() const { return m_object.begin(); }
    const_iterator end() const { return m_object.end(); }

    bool has_field(const std::string& field) const
    {
        return m_is_array ? false :m_object.find(field) != m_object.end();
    }

    void remove_field(const std::string& field)
    {
        m_is_array = false; // Ensure this is treated as an object
        m_object.erase(field);
    }

    void set_size(size_t size)
    {
        m_is_array = true; // Ensure this is treated as an array
        m_array.reserve(size);
    }

    int size() const
    {
        return m_is_array ? m_array.size() : m_object.size();
    }

    void reverse()
    {
        m_is_array = true;
        std::reverse(m_array.begin(), m_array.end());
    }

    void sort(std::function<bool(JsonNew&, JsonNew&)> compare_func)
    {
        m_is_array = true; // Ensure this is treated as an array
        std::sort(m_array.begin(), m_array.end(), compare_func);
    }

    void push_back(const JsonNew& value)
    {
        m_is_array = true; // Ensure this is treated as an array
        m_array.push_back(value);
    }

    // Methods from JsonTypeBaseNew
    virtual bool is_json_value() override
    {
        return false; // This is not a JSON value, but an object
    }

    virtual void write_string_value(JsonStringBuilder& builder) override;

    virtual JsonTypeBaseNew* get_copy() override
    {
        reference_count++;
        return this;
    }

    virtual JsonTypeBaseNew* get_deep_clone();

    virtual void release() override
    {
        reference_count--;
        if (reference_count == 0)
        {
            JsonObjectPool::release(this);
        }
    }
};