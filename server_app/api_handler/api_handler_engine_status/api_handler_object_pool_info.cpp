#include <api_handler/api_handler_engine_status/api_handler_object_pool_info.h>
#include <order/simulator_order.h>

#include <json/json.h>
#include <json/json_object.h>
#include <json/json_value.h>
#include <order_book/order_book_snapshot.h>

APIHandlerObjectPoolInfo::APIHandlerObjectPoolInfo(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerObjectPoolInfo::child_handle()
{
    Json response;

    response["data"] = {
        {"Json Object Pool Size", JsonObjectPool::size()},
        {"Json Value Pool Size", JsonValuePool::size()},
        {"Order Book Snapshot Pool Size", OrderBookSnapShotPool::size()}
    };
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);;
}