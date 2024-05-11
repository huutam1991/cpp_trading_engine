#include <api_handler/api_handler_user/api_handler_user_price_ticker_list.h>
#include <utils.h>
#include <binance_utils.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_24h_profit.h>

APIHandlerUserPriceTickerList::APIHandlerUserPriceTickerList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"from", "to"});
}

Json APIHandlerUserPriceTickerList::get_price_ticker_list(long from, long to)
{
    Json price_ticker_list = BinanceUtils::instance().get_price_ticker_by_date(m_user.get(), from, to);
    Json execution_report_list = APIHandlerBinance24hProfit::get_filled_order_list_by_day(m_user->get_active_storage_source().get(), from, to);

    execution_report_list.sort([](Json& a, Json& b){
        return (long)a["transactTime"] > (long)b["transactTime"];
    });

    Json res = Json::create_array();
    std::string status;
    std::string orderId;
    std::string val;
    execution_report_list.for_each([&res, &price_ticker_list, &status, &orderId, &val](Json& report)
    {
        orderId = std::to_string((long)report["orderId"]);
        if (price_ticker_list.has_field(orderId))
        {
            Json json = price_ticker_list[orderId].clone();
            val = (std::string&&)json["type"];
            val = val == "MS" ? "Market Scanning" : "MM Arbitrage";

            json["date_time"] = Utils::instance().get_string_time_YMD(((long long)json["transactTime"] / 1000));
            json["on_tick_time"] = Utils::instance().get_string_time_YMD_with_millisecond((long long)json["on_tick_time"]);
            json["finish_calculation_time"] = Utils::instance().get_string_time_YMD_with_millisecond((long long)json["finish_calculation_time"]);
            json["finish_place_order_time"] = Utils::instance().get_string_time_YMD_with_millisecond((long long)json["finish_place_order_time"]);
            json["strategy"] = val;
            json["tick_time"] = json["on_tick_time"];

            json.remove_field("type");
            json.remove_field("on_tick_time");
            json.remove_field("transactTime");

            res.push_back(json);
        }
    });

    return res;
}

HttpResponse APIHandlerUserPriceTickerList::child_handle()
{
    long from = std::stol(m_request->get_query_param("from"));
    long to   = std::stol(m_request->get_query_param("to"));

    Json response;
    response["data"] = get_price_ticker_list(from, to);
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}