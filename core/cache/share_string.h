#pragma once

#include <string>
#include <fmt/core.h>
#include <fmt/format.h>

#include <cache/cache_pool.h>

#define MAX_STRING_NUM 10000

struct StringReference
{
    std::string data;
    size_t count;
};

using StringPool = CachePool<StringReference, MAX_STRING_NUM>;

class ShareString
{
    StringReference* m_string_reference = nullptr;
    size_t m_start_index = 0;
    size_t m_length = 0;

public:
    ShareString() = delete;
    ShareString(const std::string& data);
    ShareString(std::string&& data);

    ShareString(const ShareString&);
    ShareString& operator=(const ShareString&);

    // Delete move constructor and move assignment operator
    // Cause the data is just pointers, no need to move
    ShareString(ShareString&&) = delete;
    ShareString& operator=(ShareString&&) = delete;

    ~ShareString();

    inline std::string_view data() const
    {
        if (!m_string_reference) return {};
        return std::string_view(m_string_reference->data).substr(m_start_index, m_length);
    }

private:
    inline void check_release_current_data();
};

template <>
struct fmt::formatter<ShareString> : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const ShareString& data, FormatContext& ctx) {
        return fmt::formatter<std::string_view>::format(data.data(), ctx);
    }
};