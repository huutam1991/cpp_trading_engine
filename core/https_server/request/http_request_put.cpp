#include <https_server/request/http_request_put.h>
#include <util_macros.h>

HttpRequestPut::HttpRequestPut(const std::string& content, const std::string& dir_path) : HttpRequestPost(content, dir_path)
{
    ADD_LOG("Create HttpRequestPut, " << m_url);
}