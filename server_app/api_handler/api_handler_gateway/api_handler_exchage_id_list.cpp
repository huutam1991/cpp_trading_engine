#include <api_handler/api_handler_gateway/api_handler_exchage_id_list.h>
#include <instrument/instrument.h>

APIHandlerExchangeIdList::APIHandlerExchangeIdList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerExchangeIdList::child_handle()
{
    Json exchnage_id_list;

    for (ExchangeId exchange_id = ExchangeId::BINANCE; exchange_id < ExchangeId::TOTAL_EXCHANGES; exchange_id = static_cast<ExchangeId>(static_cast<int>(exchange_id) + 1)  )
    {
        exchnage_id_list.push_back(enum_reflect::enum_name(exchange_id));
    }

    // Response
    Json response;
    response["data"] = exchnage_id_list;
    response["msg"] = "Get exchange ID list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}