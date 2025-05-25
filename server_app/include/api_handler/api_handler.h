#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <https_server/request/http_request.h>
#include <https_server/response/http_response.h>
#include <util_macros.h>
#include <constants.h>
#include <app_constants.h>
#include <https_server/route/route_controller.h>
#include <external_request/external_request_ssl.h>
#include <external_request/external_request.h>
#include <json/json.h>
#include <json/json_value.h>
#include <mongo_db/mongo_db_header.h>
#include <mongo_db/mongo_db.h>

class APIHandler
{
private:
    std::vector<std::string> m_mandatory_params;
    std::vector<std::string> m_mandatory_body_params;

protected:
    HttpRequest* m_request;
    bool m_need_check_authentication = false;
    bool m_need_check_none_source = false;

    virtual std::string check_authentication();
    virtual HttpResponse child_handle() = 0;

public:
    APIHandler() = delete;
    APIHandler(APIHandler&) = delete;
    APIHandler(HttpRequest* request);

    void add_mandatory_params(const std::vector<std::string>& mandatory_params);
    void add_mandatory_body_params(const std::vector<std::string>& mandatory_body_params);
    HttpResponse handle();
};

#endif //API_HANDLER_H