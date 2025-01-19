#ifndef REQUEST_FUTURE_H
#define REQUEST_FUTURE_H

#include <coroutine/future.h>
#include <external_request/external_request_ssl.h>
#include "app_utils.h"

class RequestFuture
{
private:
    std::shared_ptr<ExternalRequestSsl> m_request;

public:
    RequestFuture(const std::string& url, const std::string& port, const std::string& path, RequestMethod request_method);

    void add_header(const std::string& key, const std::string value);
    void add_body(Json& body);
    Future<Json> send_request();

};

#endif //REQUEST_FUTURE_H