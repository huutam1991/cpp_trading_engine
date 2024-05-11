#include <api_handler/api_handler_binance_spot/api_handler_binance_depth.h>
#include <utils.h>

APIHandlerBinanceDepth::APIHandlerBinanceDepth(HttpRequest* request) : APIHandlerBinance(request)
{}


Json APIHandlerBinanceDepth::get_depth_from_MongoDB_by_symbol_stream(const std::string& symbol, const std::string& limit)
{
    Json res = MongoDB::instance()
        .set_db_and_collection(BINANCE_COMMON, "depth")
        .find_one("symbol", symbol);

    if (res.is_null() == false)
    {
        int limit_int = atoi(limit.c_str());
        res["bids"].set_size(limit_int);
        res["asks"].set_size(limit_int);
        return res;
    }

    return Json();
}

Json APIHandlerBinanceDepth::get_depth_from_Binance(const std::string& symbol, const std::string& limit)
{
    Json res = send_binance_normal_request("/api/v3/depth", "symbol=" + symbol + "&limit=" + limit);
    res["symbol"] = symbol;

    return res;
}

HttpResponse APIHandlerBinanceDepth::child_handle()
{
    Json response;
    std::string symbol = m_request->get_query_param("symbol");
    std::string limit = m_request->get_query_param("limit");

    // Add default limit value (10)
    if (limit == PARAM_NOT_FOUND || limit == "")
    {
        limit = "10";
    }

    Json depth = get_depth_from_Binance(symbol, limit);

    if (depth.has_field("code") && (int)depth["code"] < 0)
    {
        response["data"] = "";
        response["msg"] = depth["msg"];
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }
    else
    {
        response["data"] = depth;
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    return HttpResponse(OK_200, response);
}