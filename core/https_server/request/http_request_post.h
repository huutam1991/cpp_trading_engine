#pragma once

#include "http_request.h"

class HttpRequestPost : public HttpRequest
{
private:
    std::string m_body;
    JsonNew m_body_json;
    void deserialize_body(const std::string& content);
    void deserialize_body_form_data();
    void deserialize_body_raw_data();

public:
    HttpRequestPost(const std::string& content, const std::string& dir_path);

    virtual bool          is_valid_format();
    virtual RequestMethod get_request_method() { return RequestMethod::POST; }
    virtual std::string   get_body();
    virtual JsonNew          get_body_json();
    virtual std::string   get_body_param_string(const std::string& param);
    virtual JsonNew          get_body_param_json(const std::string& param);
    virtual std::string   check_missing_body_params(const std::vector<std::string> fields);
};

