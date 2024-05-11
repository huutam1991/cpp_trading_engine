
#include <utils.h>
#include <binance_utils.h>
#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_exchange_info.h>

APIHandlerBinanceFuturesExchangeInfo::APIHandlerBinanceFuturesExchangeInfo(HttpRequest* request) : APIHandlerBinanceFutures(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

Json APIHandlerBinanceFuturesExchangeInfo::get_exchange_info()
{
    Json exchange_info = send_binance_normal_request("/fapi/v1/exchangeInfo", "");
    ADD_LOG("exchange_info = " << exchange_info);
    return exchange_info;
}

HttpResponse APIHandlerBinanceFuturesExchangeInfo::child_handle()
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