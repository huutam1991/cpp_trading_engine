#include <api_handler/api_handler_binance_spot/api_handler_binance_current_price.h>
#include <price_manager/price_manager.h>
#include <utils.h>

APIHandlerBinanceCurrentPrice::APIHandlerBinanceCurrentPrice(HttpRequest* request) : APIHandlerBinance(request)
{}

Json APIHandlerBinanceCurrentPrice::get_current_price_from_MongoDB_by_symbol_stream(const std::string& symbol)
{
    Json current_price = MongoDB::instance()
        .set_db_and_collection(BINANCE_COMMON, "current_price")
        .find_one("s", symbol);

    if (current_price.is_null() == false)
    {
        Json res;
        res["price"] = PriceManager::instance().get_price_by_symbol(symbol);
        res["symbol"] = symbol;

        return res;
    }

    return Json();
}

HttpResponse APIHandlerBinanceCurrentPrice::child_handle()
{
    const std::string& symbol = m_request->get_query_param("symbol");

    if (symbol != PARAM_NOT_FOUND)
    {
        Json response = {
            {"data", get_current_price_from_MongoDB_by_symbol_stream(symbol)},
            {"msg", ""},
            {"status_code", OK_200},
            {"error", false}
        };

        return HttpResponse(OK_200, response);
    }
    else
    {
        return HttpRequest::response_bad_request_400("missing symbol");
    }
}