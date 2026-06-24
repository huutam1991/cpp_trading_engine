#include <api_handler/api_handler_position/api_handler_position_list.h>
#include <gateways/gateway_manager.h>

APIHandlerPositionList::APIHandlerPositionList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerPositionList::child_handle()
{
    // Only support BINANCE for now
    auto gateway_binance = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);

    // Response
    Json response;
    response["positions"] = co_await gateway_binance->get_positions();
    response["msg"] = "Get position list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}
