#include <network/https_server/request/http_request_get.h>

HttpRequestGet::HttpRequestGet(const std::string& content, const std::string& dir_path) : HttpRequest(content, dir_path)
{
    spdlog::debug("Create HttpRequestGet, {}", m_url);
}
