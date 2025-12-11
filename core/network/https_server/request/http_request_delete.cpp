#include <network/https_server/request/http_request_delete.h>

HttpRequestDelete::HttpRequestDelete(const std::string& content, const std::string& dir_path) : HttpRequestPost(content, dir_path)
{
    spdlog::debug("Create HttpRequestDelete, {}", m_url);
}
