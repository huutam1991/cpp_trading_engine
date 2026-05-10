#include <api_handler/api_handler_instrument/api_handler_instrument_subscribe.h>
#include <mongo_db/mongo_db.h>
#include <app_utils/app_utils.h>
#include <spdlog/spdlog.h>

APIHandlerInstrumentSubscribe::APIHandlerInstrumentSubscribe(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"username", "password"});
}

Task<HttpResponse> APIHandlerInstrumentSubscribe::child_handle()
{
    Json response;

    if (m_request->get_request_method() == RequestMethod::GET)
    {
        response["data"] = "This is a GET request";
    }
    else if (m_request->get_request_method() == RequestMethod::POST)
    {
        response["data"] = "This is a POST request";
    }

    // Response
    response["data"] = {};
    response["msg"] = "register account ";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}
