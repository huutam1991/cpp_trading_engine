#pragma once

#include <c_json/json_type_base.h>
#include <c_json/json_value.h>

#define STRING_BUFFER_SIZE 100000 // Reserve space for 100000 characters

class JsonNew
{
    JsonTypeBaseNew* m_value = nullptr;

public:
    JsonNew();

    JsonNew(const JsonNew& copy) noexcept
    {
        m_value = copy.m_value ? copy.m_value->get_copy() : nullptr;
    }

    JsonNew(JsonNew&& copy) noexcept
    {
        // Transfer ownership
        m_value = copy.m_value;
        copy.m_value = nullptr;
    }

    JsonNew(std::initializer_list<std::pair<std::string, JsonNew>> json_list);

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    JsonNew(T&& value)
    {
        check_create_json_value();
        ((JsonValueNew*)m_value)->operator=(std::forward<T>(value));
    }

    static JsonNew parse(std::string json_string);

    JsonNew& operator=(const JsonNew& copy) noexcept
    {
        if (this != &copy)
        {
            if (m_value)
            {
                m_value->release();
            }

            m_value = copy.m_value ? copy.m_value->get_copy() : nullptr;
        }
        return *this;
    }

    JsonNew& operator=(JsonNew&& copy) noexcept
    {
        if (this != &copy)
        {
            if (m_value)
            {
                m_value->release();
            }

            m_value = copy.m_value;
            copy.m_value = nullptr; // Transfer ownership
        }
        return *this;
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    JsonNew& operator=(T&& value)
    {
        check_create_json_value();
        ((JsonValueNew*)m_value)->operator=(std::forward<T>(value));
        return *this;
    }

    template<class T>
    operator T()
    {
        if (m_value == nullptr || m_value->is_json_value() == false)
        {
            return T(); // Return default value if not a valid JsonValue
        }
        else
        {
            return ((JsonValueNew*)m_value)->operator T();
        }
    }

    JsonNew& operator[](const char* key);
    JsonNew& operator[](const std::string& key);
    JsonNew& operator[](size_t index);

    template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    JsonNew& operator[](T index)
    {
        return (*this)[static_cast<size_t>(index)];
    }

    bool operator==(const JsonNew& other) const
    {
        // Check if both m_value pointers are the same
        return m_value == other.m_value;
    }

    bool operator!=(const JsonNew& other) const
    {
        // Check if both m_value pointers are the same
        return m_value != other.m_value;
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    bool operator ==(T value) const
    {
        if (m_value == nullptr || m_value->is_json_value() == false)
        {
            return false; // If m_value is null or not a JsonValue, return false
        }
        else
        {
            if constexpr (std::is_same_v<std::decay_t<T>, const char*>)
            {
                const ShareString& share_string = ((JsonValueNew*)m_value)->operator ShareString();
                const char* current_value = share_string.data().data();
                return std::strcmp(current_value, value) == 0;
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>)
            {
                const ShareString& share_string = ((JsonValueNew*)m_value)->operator ShareString();
                std::string_view current_value = share_string.data();
                return current_value == value;
            }
            else
            {
                T current_value = ((JsonValueNew*)m_value)->operator T();
                return current_value == value;
            }
        }
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    bool operator !=(T value) const
    {
        // Reuse == operator
        return operator==(value) == false;
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    bool operator <(const T& value) const
    {
        if (m_value == nullptr || m_value->is_json_value() == false)
        {
            return false;
        }
        else
        {
            T current_value = ((JsonValueNew*)m_value)->operator T();
            return current_value < value;
        }
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    bool operator >(const T& value) const
    {
        if (m_value == nullptr || m_value->is_json_value() == false)
        {
            return false;
        }
        else
        {
            T current_value = ((JsonValueNew*)m_value)->operator T();
            return current_value > value;
        }
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    bool operator <=(const T& value) const
    {
        // Reuse > operator
        return operator>(value) == false;
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    bool operator >=(const T& value) const
    {
        // Reuse < operator
        return operator<(value) == false;
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    JsonNew& operator +=(const T& value)
    {
        check_create_json_value();
        T current_value = ((JsonValueNew*)m_value)->operator T();
        current_value += value;
        ((JsonValueNew*)m_value)->operator=(current_value);
        return *this;
    }

    template <class T, std::enable_if_t<!std::is_same<std::decay_t<T>, JsonNew>::value, int> = 0>
    JsonNew& operator -=(const T& value)
    {
        operator+=(-value); // Reuse += operator
        return *this;
    }

    void for_each(std::function<void(JsonNew&)> loop_func);
    void for_each_with_key(std::function<void(const std::string&,JsonNew&)> loop_func);
    void for_each_with_index(std::function<void(size_t,JsonNew&)> loop_func);

    using iterator = std::unordered_map<std::string, JsonNew>::iterator;
    using const_iterator = std::unordered_map<std::string, JsonNew>::const_iterator;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    bool has_field(const std::string& field) const;
    void remove_field(const std::string& field);

    void set_is_string_format(bool val)
    {
        if (m_value)
        {
            ((JsonValueNew*)m_value)->set_is_string_format(val);
        }
    }

    bool is_string() const
    {
        return m_value == nullptr ?
            false :
            m_value->is_json_value() ?
                ((JsonValueNew*)m_value)->is_string() :
                false;
    }

    bool is_array() const;
    bool is_object() const;

    void set_size(size_t size);
    void set_capacity(size_t size);
    int size() const;
    int capacity() const;
    void reverse();
    void sort(std::function<bool(JsonNew&, JsonNew&)> compare_func);
    void push_back(const JsonNew& value);

    JsonNew deep_clone()
    {
        JsonNew res;
        res.m_value = m_value ? m_value->get_deep_clone() : nullptr;
        return res;
    }

    // Null check
    bool operator==(std::nullptr_t t) const
    {
        return is_null();
    }

    ~JsonNew()
    {
        if (m_value != nullptr)
        {
            m_value->release();
            m_value = nullptr;
        }
    }

    std::string get_string_value() const
    {
        char buffer[STRING_BUFFER_SIZE];

        // Write the JSON value to the buffer
        JsonStringBuilder builder(buffer);
        write_string_value(builder);

        return builder.finish();
    }

    void write_string_value(JsonStringBuilder& builder) const
    {
        if (m_value)
        {
            m_value->write_string_value(builder);
        }
        else
        {
            builder.write_raw("null", 4); // Write "null" if m_value is nullptr
        }
    }

private:
    bool is_null() const;
    void check_create_json_value();
    void check_create_json_object();
};

template <>
struct fmt::formatter<JsonNew> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const JsonNew& json, FormatContext& ctx)
    {
        return fmt::formatter<std::string>::format(json.get_string_value(), ctx);
    }
};