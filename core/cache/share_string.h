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

    const std::string& data() const { return m_string_reference->data; }

private:
    inline void check_release_current_data();
};

template <>
struct fmt::formatter<ShareString> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const ShareString& data, FormatContext& ctx) {
        return fmt::formatter<std::string>::format(data.data(), ctx);
    }
};