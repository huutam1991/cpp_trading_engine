#pragma once

#include <string>
#include <functional>
#include <unordered_map>

#include <coroutine/task.h>
#include <network/https_server/response/http_response.h>

class HttpRequest;
using RequestHandleFunction = std::function<Task<HttpResponse>(HttpRequest*)>;

class HttpRequest
{
private:
    static std::function<HttpResponse(HttpRequest*)> s_bad_request_getter;

    void deserialize(const std::string& content);
    void deserialize_url(const std::string& content);
    void deserialize_query_string(const std::string& content);
    void deserialize_query_params(const std::string& content);
    void deserialize_header_params(const std::string& content);
    ResponseFileType check_file_type(const std::string& name);

protected:
    std::string m_url;
    std::string m_dir_path;
    std::string m_query_string;
    std::string m_content_type;

    std::unordered_map<std::string, std::string> m_query_params;
    std::unordered_map<std::string, std::string> m_header_params;

public:
    HttpRequest(const std::string& content, const std::string& dir_path);
    virtual ~HttpRequest() = default;

    virtual RequestMethod get_request_method() { return RequestMethod::UNKNOWN; }
    const std::string& get_url();
    const std::string& get_query_param(const std::string& param);
    const std::string& get_query_string();
    const std::string  get_header_param(const std::string& param);
    const std::string  check_missing_params(const std::vector<std::string>& params);
    Json get_query_json();
    static const std::string  get_query_string_from_query_json(Json& query_object);

    virtual bool        is_valid_format() { return true; }
    virtual std::string get_body() { return std::string(""); }
    virtual Json        get_body_json() { return nullptr; }
    virtual std::string get_body_param_string(const std::string& param) { return std::string(""); }
    virtual Json        get_body_param_json(const std::string& param) { return nullptr; }
    virtual std::string check_missing_body_params(const std::vector<std::string> fields) { return PARAM_NO_MISSING; }
    bool                check_is_file_path_exist(const std::string& file_path);
    HttpResponse send_file_from_directory(const std::string& url, const FileInfo& file_info);
    HttpResponse send_file_from_directory(const std::string& url, const std::string& file_name = "");
    HttpResponse send_file_from_directory(const std::string& url, ResponseFileType file_type);
    HttpResponse response_not_found_404();

    static HttpResponse response_bad_request_400(const std::string& error);
    static HttpResponse response_unauthorized_request_401(const std::string& error);
    static HttpResponse response_internal_error_500();
    static HttpRequest* CreateNewHttpRequest(const std::string& content, const std::string& dir_path);
    static void add_custom_bad_request_getter(std::function<HttpResponse(HttpRequest*)> bad_request_getter);
};

