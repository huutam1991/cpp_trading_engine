#include <network/https_server/request/http_request_post.h>
#include <utils/utils.h>

HttpRequestPost::HttpRequestPost(const std::string& content, const std::string& dir_path) : HttpRequest(content, dir_path)
{
    spdlog::debug("Create HttpRequestPost, {}", m_url);
    deserialize_body(content);
    deserialize_body_form_data();
    deserialize_body_raw_data();
}

void HttpRequestPost::deserialize_body(const std::string& content)
{
    size_t end_of_request_header = content.find("\r\n\r\n", 0);
    m_body = content.substr(end_of_request_header + 4, content.size() - 1);
    m_body_json = Json::parse(m_body);
    spdlog::debug("body = {}", m_body);
}

void HttpRequestPost::deserialize_body_form_data()
{
    std::string key_format = "Content-Disposition: form-data; name=";
    int form_data_start = m_body.find(key_format);

    if (form_data_start > -1)
    {
        std::string frame_format = m_body.substr(0, form_data_start);
        std::string key;
        std::string value;
        int start = 0;
        int key_start;
        int key_end;
        int value_start;
        int value_end;

        while ((key_start = m_body.find(key_format, start)) > - 1)
        {
            key_start +=  key_format.size() + 1;
            key_end = m_body.find("\"", key_start);
            key = m_body.substr(key_start, key_end - key_start);

            value_start = m_body.find("\r\n\r\n", key_end) + 4;
            value_end = m_body.find("\r\n", value_start);
            value = m_body.substr(value_start, value_end - value_start);

            m_body_json[key] = value;

            start = m_body.find(frame_format, start + frame_format.size());
        }
    }
}

void HttpRequestPost::deserialize_body_raw_data()
{
    if (m_body_json == nullptr)
    {
        std::vector<std::string> param_arr;
        std::vector<std::string> body_arr = Utils::split_string(m_body, "&");
        for (int i = 0; i < body_arr.size(); i++)
        {
            param_arr = Utils::split_string(body_arr[i], "=");
            if (param_arr.size() >= 2)
            {
                m_body_json[param_arr[0]] = param_arr[1];
            }
        }
    }
}

bool HttpRequestPost::is_valid_format()
{
    int content_length = 0;

    std::string param_content_length_1 = get_header_param("Content-Length");
    std::string param_content_length_2 = get_header_param("content-length");
    if (param_content_length_1 != PARAM_NOT_FOUND)
    {
        content_length = stoi(param_content_length_1);
    }
    else if (param_content_length_2 != PARAM_NOT_FOUND)
    {
        content_length = stoi(param_content_length_2);
    }

    return content_length == 0 || content_length == m_body.size();
}

std::string HttpRequestPost::get_body()
{
    return m_body;
}

Json HttpRequestPost::get_body_json()
{
    return m_body_json;
}

std::string HttpRequestPost::get_body_param_string(const std::string& param)
{
    if (m_body_json.has_field(param))
    {
        return m_body_json[param];
    }

    return PARAM_NOT_FOUND;
}

Json HttpRequestPost::get_body_param_json(const std::string& param)
{
    if (m_body_json.has_field(param))
    {
        return m_body_json[param];
    }

    return nullptr;
}

std::string HttpRequestPost::check_missing_body_params(const std::vector<std::string> fields)
{
    for (int i = 0; i < fields.size(); i++)
    {
        if (m_body_json.has_field(fields[i]) == false)
        {
            return "Missing body param: [" + fields[i] + "]";
        }
    }

    return PARAM_NO_MISSING;
}