#include <https_server/request/http_request_get.h>
#include <util_macros.h>

HttpRequestGet::HttpRequestGet(const std::string& content, const std::string& dir_path) : HttpRequest(content, dir_path)
{
    ADD_LOG("Create HttpRequestGet, " << m_url);
}
