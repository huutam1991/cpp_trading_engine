#include <json/json.h>
#include <utils/utils.h>

#include <api_handler/api_handler_system_monitoring/api_handler_up_time.h>

APIHandlerUpTime::APIHandlerUpTime(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerUpTime::child_handle()
{
    Json response;
    response["msg"] = "Get up time successfully";
    response["data"] = Utils::get_up_time();
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);;
}