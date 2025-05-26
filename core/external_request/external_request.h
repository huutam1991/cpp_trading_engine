#pragma once

#include <string>
#include <utils/constants.h>

class ExternalRequest
{
public:
    ExternalRequest(const std::string& url, int port, const std::string& path, RequestMethod request_method);
    ExternalRequest(ExternalRequest&) = delete;

protected:
    std::string m_url;
    std::string m_path;
    int m_port;
    RequestMethod m_request_method;

public:
    std::string send_request(bool wait_response = true);

};
