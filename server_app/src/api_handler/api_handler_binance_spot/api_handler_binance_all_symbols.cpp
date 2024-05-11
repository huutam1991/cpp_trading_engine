#include <utils.h>
#include <binance_utils.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_all_symbols.h>

APIHandlerBinanceAllSymbols::APIHandlerBinanceAllSymbols(HttpRequest* request) : APIHandlerBinance(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

HttpResponse APIHandlerBinanceAllSymbols::child_handle()
{
    Json response = {
        {"data", BinanceUtils::instance().get_all_symbols_by_db_name(m_user->get_storage_source_db_name())},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false},
    };

    return HttpResponse(OK_200, response);
}