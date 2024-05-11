#include <request/http_request_delete.h>
#include <util_macros.h>

HttpRequestDelete::HttpRequestDelete(const std::string& content, const std::string& dir_path) : HttpRequestPost(content, dir_path)
{
    ADD_LOG("Create HttpRequestDelete, " << m_url);
}
