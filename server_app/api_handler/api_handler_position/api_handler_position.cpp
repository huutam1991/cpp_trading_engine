#include <api_handler/api_handler_position/api_handler_position.h>
#include <gateways/gateway_manager.h>

APIHandlerPosition::APIHandlerPosition(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerPosition::child_handle()
{
    // Only support BINANCE for now
    auto gateway_binance = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);

    // Response
    Json response;
    response["orders"] = co_await gateway_binance->get_positions();
    response["msg"] = "Get order list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}
