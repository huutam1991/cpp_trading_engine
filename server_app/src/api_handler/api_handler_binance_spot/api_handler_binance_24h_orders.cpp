#include <utils.h>
#include <binance_utils.h>
#include <order_manager/order_manager.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_24h_orders.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_current_price.h>

APIHandlerBinance24hOrders::APIHandlerBinance24hOrders(HttpRequest* request) : APIHandlerBinance(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

Json APIHandlerBinance24hOrders::get_orders_list_in_24h()
{
    long today_0h = Utils::instance().get_0h_today_in_utc() * 1000;
    long tomorrow_0h = Utils::instance().get_0h_tomorrow_in_utc() * 1000;

    return OrderManager::instance().get_order_list(
        m_user->get_user_id(),
        m_user->get_storage_source_db_name(),
        today_0h,
        tomorrow_0h
    );
}

Json APIHandlerBinance24hOrders::get_orders_list_by_time(long from, long to)
{
    return OrderManager::instance().get_order_list(
        m_user->get_user_id(),
        m_user->get_storage_source_db_name(),
        from,
        to
    );
}

HttpResponse APIHandlerBinance24hOrders::child_handle()
{
    Json data;
    Json query_json = m_request->get_query_json();

    if (query_json.has_field("from") && query_json.has_field("to"))
    {
        long from = std::stol(m_request->get_query_param("from"));
        long to   = std::stol(m_request->get_query_param("to"));

        ADD_LOG("get order list from to");
        data = get_orders_list_by_time(from, to);
    }
    else
    {
        ADD_LOG("get order list 24h ");
        data = get_orders_list_in_24h();
    }

    Json response = {
        {"data", data},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false},
    };

    return HttpResponse(OK_200, response);
}