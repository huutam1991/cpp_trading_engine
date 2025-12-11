#include <iostream>
#include <fstream>
#include <filesystem>

#include <network/https_server/response/http_response.h>

HttpResponse::HttpResponse() : m_response_code(OK_200)
{
    m_content = new std::string();
}

HttpResponse::HttpResponse(ResponseStatusCode response_code, const Json& json) : m_response_code(response_code)
{
    m_content = new std::string(json.get_string_value());
    m_is_json_format = true;
}

HttpResponse::HttpResponse(ResponseStatusCode response_code, const std::string& content, const std::string& file_name) : m_response_code(response_code)
{
    m_content = new std::string(content);
    m_file_info.file_name = file_name;
}

HttpResponse::HttpResponse(ResponseStatusCode response_code, const std::string& content, ResponseFileType file_type) : m_response_code(response_code)
{
    m_content = new std::string(content);
    m_file_info.file_type = file_type;
}

HttpResponse::HttpResponse(ResponseStatusCode response_code, const std::string& content, const FileInfo& file_info) : m_response_code(response_code)
{
    m_content = new std::string(content);
    m_file_info = file_info;
}

HttpResponse::HttpResponse(HttpResponse& response)
    : m_response_code(response.m_response_code),
      m_content(response.m_content),
      m_is_json_format(response.m_is_json_format),
      m_custom_header(response.m_custom_header),
      m_file_info(response.m_file_info)
{
    response.m_content = nullptr;
}

HttpResponse::HttpResponse(const HttpResponse& response)
    : m_response_code(response.m_response_code),
      m_content{new std::string(*response.m_content)},
      m_is_json_format(response.m_is_json_format),
      m_custom_header(response.m_custom_header),
      m_file_info(response.m_file_info)
{
    // [response.m_content] suppose to be valid
    // response.m_content = nullptr;
}

HttpResponse::~HttpResponse()
{
    if (m_content != nullptr)
    {
        delete m_content;
        m_content = nullptr;
    }
}

const HttpResponse& HttpResponse::operator=(HttpResponse& response)
{
    m_response_code = response.m_response_code;
    m_content = response.m_content;
    m_custom_header = response.m_custom_header;
    m_is_json_format = response.m_is_json_format;
    m_file_info = response.m_file_info;

    // Untrack m_content of old HttpResponse
    response.m_content = nullptr;
    return *this;
}

const HttpResponse& HttpResponse::operator=(HttpResponse&& response)
{
    m_response_code = response.m_response_code;
    m_content = response.m_content;
    m_custom_header = response.m_custom_header;
    m_is_json_format = response.m_is_json_format;
    m_file_info = response.m_file_info;

    // Untrack m_content of old HttpResponse
    response.m_content = nullptr;
    return *this;
}

void HttpResponse::add_custom_header(Json& custom_header)
{
    m_custom_header = custom_header;
}

std::string HttpResponse::get_response_in_string()
{
    auto status_code_pair = response_status_code_map.find(m_response_code);
    std::string response;

    // Response line
    response.append(HTTP_VERSION);
    response.append(" ");
    response.append(std::to_string(status_code_pair->first));
    response.append(" ");
    response.append(status_code_pair->second);
    response.append(LINE_ENDING);

    // Custom header
    if (m_custom_header != nullptr)
    {
        m_custom_header.for_each_with_key([&response](const std::string& header_name, Json& header_data)
        {
            response.append(header_name);
            response.append(": ");
            response.append(header_data.get_string_value());
            response.append(LINE_ENDING);
        });
    }

    // CORS
    response.append("Access-Control-Allow-Origin: *");
    response.append(LINE_ENDING);
    response.append("Access-Control-Allow-Headers: Origin, X-Api-Key, X-Requested-With, Content-Type, Accept, Authorization");
    response.append(LINE_ENDING);
    response.append("Access-Control-Allow-Credentials: true");
    response.append(LINE_ENDING);
    response.append("Access-Control-Allow-Methods: GET, POST, OPTIONS, PUT, PATCH, DELETE");
    response.append(LINE_ENDING);

    // Content
    if (m_is_json_format == true)
    {
        response.append("Content-Type: application/json");
        response.append(LINE_ENDING);
    }
    else if (m_file_info.file_type == ResponseFileType::JS)
    {
        response.append("Content-Type: application/javascript; charset=utf-8");
        response.append(LINE_ENDING);
    }
    else if (m_file_info.file_type == ResponseFileType::CSS)
    {
        response.append("Content-Type: text/css; charset=utf-8");
        response.append(LINE_ENDING);
    }
    else if (m_file_info.file_type == ResponseFileType::WOFF2)
    {
        response.append("Content-Type: font/woff2");
        response.append(LINE_ENDING);
    }

    if (m_file_info.file_name != "")
    {
        response.append("Content-Disposition: attachment; filename=\"" + m_file_info.file_name + "\"");
        response.append(LINE_ENDING);
    }
    response.append("Content-Length: ");
    response.append(std::to_string(m_content->size()));
    response.append(LINE_ENDING);
    response.append(LINE_ENDING);
    response.append(*m_content);

    return response;
}
