#include <api_handler/api_handler_gateway/api_handler_gateway_list.h>
#include <gateways/gateway_manager.h>

APIHandlerGatewayList::APIHandlerGatewayList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerGatewayList::child_handle()
{
    std::vector<std::shared_ptr<Gateway>> gateways = GatewayManager::instance().get_all_gateways();

    // Response
    Json response;
    response["data"] = gateways;
    response["msg"] = "Get gateway list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}