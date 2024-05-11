#include <api_handler/api_handler_coinbase/api_handler_coinbase_current_price.h>
#include <utils.h>

APIHandlerCoinbaseCurrentPrice::APIHandlerCoinbaseCurrentPrice(HttpRequest* request) : APIHandlerCoinbase(request)
{
    add_mandatory_params({"symbol"});
}

Json APIHandlerCoinbaseCurrentPrice::get_current_price_from_MongoDB_by_symbol_stream(const std::string& symbol)
{
    // Json current_price = MongoDB::instance()
    //     .set_db_and_collection(BINANCE_COMMON, "current_price")
    //     .find_one("s", symbol);

    // if (current_price.is_null() == false)
    // {
    //     Json res;
    //     res["high_price"] = std::stold((std::string&&)current_price["h"]);
    //     res["low_price"] = std::stold((std::string&&)current_price["l"]);
    //     res["open_price"] = std::stold((std::string&&)current_price["o"]);
    //     res["close_price"] = std::stold((std::string&&)current_price["c"]);

    //     return res;
    // }

    return Json();
}

HttpResponse APIHandlerCoinbaseCurrentPrice::child_handle()
{
    std::string symbol = m_request->get_query_param("symbol");
    Json oracle = send_coinbase_normal_request("/oracle", "");

    ADD_LOG("oracle : " << oracle);

    Json response = {
        {"data", {
            {"price", (std::string&&)oracle["prices"][symbol]},
            {"symbol", symbol}
        }},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false}
    };

    return HttpResponse(OK_200, response);
}