#include <network/https_server/request/http_request_put.h>

HttpRequestPut::HttpRequestPut(const std::string& content, const std::string& dir_path) : HttpRequestPost(content, dir_path)
{
    // spdlog::debug("Create HttpRequestPut, {}", m_url);
}