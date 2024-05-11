#include <utils.h>
#include <binance_utils.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_exchange_info.h>

APIHandlerBinanceExchangeInfo::APIHandlerBinanceExchangeInfo(HttpRequest* request) : APIHandlerBinance(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;

    add_mandatory_params({"symbol"});
}

Json APIHandlerBinanceExchangeInfo::get_symbol_info()
{
    Json exchange_info = send_binance_normal_request("/api/v3/exchangeInfo", "");
    ADD_LOG("exchange_info = " << exchange_info);
    return exchange_info["symbols"][0];
}

HttpResponse APIHandlerBinanceExchangeInfo::child_handle()
{
    std::string symbol = m_request->get_query_param("symbol");

    Json response = {
        {"data", BinanceUtils::instance().get_symbol_info(symbol)},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false},
    };

    return HttpResponse(OK_200, response);
}