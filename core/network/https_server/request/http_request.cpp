#include <iostream>
#include <fstream>
#include <filesystem>

#include <network/https_server/request/http_request.h>
#include <network/https_server/request/http_request_get.h>
#include <network/https_server/request/http_request_post.h>
#include <network/https_server/request/http_request_put.h>
#include <network/https_server/request/http_request_delete.h>
#include <utils/constants.h>
// #include <utils/util_macros.h>
#include <utils/utils.h>

std::unordered_map<RequestMethod, std::function< HttpRequest*(const std::string&, const std::string&)>> request_generator_by_method =
{
    {RequestMethod::GET     , [](const std::string& content, const std::string& dir_path){ return new HttpRequestGet(content, dir_path); } },
    {RequestMethod::POST    , [](const std::string& content, const std::string& dir_path){ return new HttpRequestPost(content, dir_path); } },
    {RequestMethod::DELETE  , [](const std::string& content, const std::string& dir_path){ return new HttpRequestDelete(content, dir_path); } },
    {RequestMethod::OPTIONS , [](const std::string& content, const std::string& dir_path){ return new HttpRequest(content, dir_path); } },
    {RequestMethod::HEAD    , [](const std::string& content, const std::string& dir_path){ return new HttpRequest(content, dir_path); } },
    {RequestMethod::PUT     , [](const std::string& content, const std::string& dir_path){ return new HttpRequestPut(content, dir_path); } },
    {RequestMethod::PATCH   , [](const std::string& content, const std::string& dir_path){ return new HttpRequest(content, dir_path); } },
};

std::function<HttpResponse(HttpRequest*)> bad_request_default = [](HttpRequest* request) -> HttpResponse
{
    return HttpResponse(NOT_FOUND_404, NOT_FOUND_ERROR_MESSAGE);
};

// Init default bad request 404's response
std::function<HttpResponse(HttpRequest*)> HttpRequest::s_bad_request_getter = bad_request_default;

HttpRequest::HttpRequest(const std::string& content, const std::string& dir_path) : m_dir_path(dir_path)
{
    deserialize(content);
}

void HttpRequest::deserialize(const std::string& content)
{
    deserialize_url(content);
    deserialize_query_string(content);
    deserialize_query_params(content);
    deserialize_header_params(content);
}

void HttpRequest::deserialize_url(const std::string& content)
{
    size_t end_of_method_pos = content.find_first_of(' ', 0);
    size_t end_of_url_pos = content.find_first_of(' ', end_of_method_pos + 1);
    if (end_of_url_pos == std::string::npos)
    {
        spdlog::error("deserialize_url No URL substring found");
        return;
    }
    m_url = content.substr(end_of_method_pos + 1, end_of_url_pos - end_of_method_pos - 1);

    // spdlog::debug("Route = {}", m_url);
}

void HttpRequest::deserialize_query_string(const std::string& content)
{
    int question_mark_pos = m_url.find("?", 0);
    if (question_mark_pos != -1)
    {
        m_query_string = m_url.substr(question_mark_pos + 1, m_url.size() - 1);
        m_url = m_url.substr(0, question_mark_pos);
    }
    // spdlog::debug("query = \"{}\"", m_query_string);
}

void HttpRequest::deserialize_query_params(const std::string& content)
{
    size_t query_string_size = m_query_string.size();
    if (query_string_size > 0)
    {
        size_t start = 0;
        size_t end = 0;

        while (start < query_string_size)
        {
            while (m_query_string[end] != '&' && end != query_string_size)
            {
                end++;
            }

            std::string param = m_query_string.substr(start, end - start);
            int equal_pos = param.find("=", 0);
            if (equal_pos < 0) break;

            std::string key = param.substr(0, equal_pos);
            std::string value = param.substr(equal_pos + 1, param.size() - 1);

            m_query_params.insert(std::make_pair(key, value));

            end++;
            start = end;
        }
    }
}

void HttpRequest::deserialize_header_params(const std::string& content)
{
    size_t start_of_header = content.find_first_of("\r\n", 0) + 2;
    size_t end_of_header = content.find("\r\n\r\n", 0);

    std::vector<std::string> header_list = Utils::split_string(
        content.substr(start_of_header, end_of_header - start_of_header),
        "\r\n"
    );

    std::vector<std::string> header;
    for (int i = 0; i < header_list.size(); i++)
    {
        header = Utils::split_string(header_list[i], ": ");
        if (header.size() == 2)
        {
            m_header_params.insert(std::make_pair(header[0], header[1]));
        }
    }
}

ResponseFileType HttpRequest::check_file_type(const std::string& name)
{
    if (name != "")
    {
        if (name.find(".js") != -1)
        {
            return ResponseFileType::JS;
        }
        else if (name.find(".css") != -1)
        {
            return ResponseFileType::CSS;
        }
        else if (name.find(".woff2") != -1)
        {
            return ResponseFileType::WOFF2;
        }
    }

    return ResponseFileType::NONE_TYPE;
}

const std::string& HttpRequest::get_url()
{
    return m_url;
}

const std::string& HttpRequest::get_query_param(const std::string& param)
{
    auto it = m_query_params.find(param);
    if (it != m_query_params.end())
    {
        return it->second;
    }

    return PARAM_NOT_FOUND;
}

const std::string& HttpRequest::get_query_string()
{
    return m_query_string;
}

const std::string HttpRequest::get_query_string_from_query_json(Json& query_object)
{
    std::string res;
    int counter = 0;

    query_object.for_each_with_key([&res, &counter](const std::string& key, Json& param)
    {
        if (param.is_string())
        {
            // There is no string format in query string (ex: ?key="value")
            param.set_is_string_format(false);
        }
        std::string param_str = param.get_string_value();
        res += (counter++ > 0 ? "&" : "") + key + "=" + param_str;
    });

    return res;
}

const std::string HttpRequest::get_header_param(const std::string& param)
{
    auto it = m_header_params.find(param);
    if (it != m_header_params.end())
    {
        return it->second;
    }

    return PARAM_NOT_FOUND;
}

Json HttpRequest::get_query_json()
{
    Json res;
    for (auto it = m_query_params.begin(); it != m_query_params.end(); it++)
    {
        res[it->first] = it->second;
    }

    return res;
}

const std::string HttpRequest::check_missing_params(const std::vector<std::string>& params)
{
    for (int i = 0; i < params.size(); i++)
    {
        if (get_query_param(params[i]) == PARAM_NOT_FOUND)
        {
            return "Missing query param: [" + params[i] + "]";
        }
    }

    return PARAM_NO_MISSING;
}

bool HttpRequest::check_is_file_path_exist(const std::string& file_path)
{
    std::string path = std::string(m_dir_path + "/" + file_path);
    std::ifstream ifs(path);

    return ifs.good() && std::filesystem::is_directory(path) == false;
}

HttpResponse HttpRequest::send_file_from_directory(const std::string& url, const FileInfo& file_info)
{
    HttpResponse response;

    std::string file_path = std::string(m_dir_path + "/" + url);
    std::ifstream ifs(file_path);

    if (ifs.good() && std::filesystem::is_directory(file_path) == false)
    {
        std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        response = HttpResponse(OK_200, content, file_info);
    }
    else if (!std::filesystem::exists(file_path))
    {
        response = bad_request_default(this);
    }

    return response;
}

HttpResponse HttpRequest::send_file_from_directory(const std::string& url, const std::string& file_name)
{
    FileInfo file_info;
    file_info.file_name = file_name;
    file_info.file_type = file_name != "" ? check_file_type(file_name) : check_file_type(url);

    return send_file_from_directory(url, file_info);
}

HttpResponse HttpRequest::send_file_from_directory(const std::string& url, ResponseFileType file_type)
{
    FileInfo file_info;
    file_info.file_type = file_type;

    return send_file_from_directory(url, file_info);
}

HttpResponse HttpRequest::response_not_found_404()
{
    if (this != nullptr)
    {
        return HttpRequest::s_bad_request_getter(this);
    }

    return bad_request_default(this);
}


HttpResponse HttpRequest::response_bad_request_400(const std::string& error)
{
    Json response;
    response["data"] = "";
    response["msg"] = error;
    response["status_code"] = BAD_REQUEST_400;
    response["error"] = true;

    return HttpResponse(ResponseStatusCode::BAD_REQUEST_400, response);
}

HttpResponse HttpRequest::response_unauthorized_request_401(const std::string& error)
{
    Json response;
    response["data"] = "";
    response["msg"] = error;
    response["status_code"] = UNAUTHORIZED_REQUEST_401;
    response["error"] = true;

    return HttpResponse(ResponseStatusCode::UNAUTHORIZED_REQUEST_401, response);
}

HttpResponse HttpRequest::response_internal_error_500()
{
    Json response;
    response["data"] = "";
    response["msg"] = "Internal Server Error";
    response["status_code"] = INTERNAL_SERVER_ERROR_500;
    response["error"] = true;

    return HttpResponse(ResponseStatusCode::INTERNAL_SERVER_ERROR_500, response);
}

void HttpRequest::add_custom_bad_request_getter(std::function<HttpResponse(HttpRequest*)> bad_request_getter)
{
    HttpRequest::s_bad_request_getter = bad_request_getter;
}

HttpRequest* HttpRequest::CreateNewHttpRequest(const std::string& content, const std::string& dir_path)
{
    // // Get String method of request
    // spdlog::debug("Content = {}", content);

    size_t end_of_method_pos = content.find_first_of(' ', 0);
    if (end_of_method_pos == std::string::npos) {
        spdlog::error("No HTTP-Method substring found");
        // exit(EXIT_FAILURE);
        return nullptr;
    }
    std::string request_method_str = content.substr(0, end_of_method_pos);

    // Get Enum method of request
    auto method_it = request_method_map.find(request_method_str);
    if (method_it == request_method_map.end()) {
        spdlog::error("No valid HTTP-Method found: {}", request_method_str);
        // exit(EXIT_FAILURE);
        return nullptr;
    }
    RequestMethod method = method_it->second;

    // Get request generator
    std::function<HttpRequest*(const std::string&, const std::string&)> request_generator = request_generator_by_method.find(method)->second;

    // Return new HttpRequest
    return request_generator(content, dir_path);
}


