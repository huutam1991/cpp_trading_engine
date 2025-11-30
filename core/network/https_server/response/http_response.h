#pragma once

#include <string>
#include <functional>

#include <utils/constants.h>
#include <json/json.h>

enum ResponseFileType
{
    JS,
    CSS,
    WOFF2,
    NONE_TYPE,
};

struct FileInfo
{
    ResponseFileType file_type = ResponseFileType::NONE_TYPE;
    std::string      file_name = "";
};

class HttpResponse
{
private:
    ResponseStatusCode m_response_code;
    Json m_custom_header = nullptr;
    const std::string* m_content;
    bool m_is_json_format = false;
    FileInfo m_file_info;

protected:

public:
    HttpResponse();
    HttpResponse(HttpResponse& response);
    HttpResponse(const HttpResponse& response);
    HttpResponse(ResponseStatusCode response_code, const Json& json);
    HttpResponse(ResponseStatusCode response_code, const std::string& content, const std::string& file_name);
    HttpResponse(ResponseStatusCode response_code, const std::string& content, ResponseFileType file_type = ResponseFileType::NONE_TYPE);
    HttpResponse(ResponseStatusCode response_code, const std::string& content, const FileInfo& file_info);
    ~HttpResponse();

    const HttpResponse& operator=(HttpResponse& response);
    const HttpResponse& operator=(HttpResponse&& response);
    virtual std::string get_response_in_string();

    void add_custom_header(Json& custom_header);
};
