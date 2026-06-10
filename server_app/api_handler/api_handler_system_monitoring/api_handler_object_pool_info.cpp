#include <api_handler/api_handler_system_monitoring/api_handler_object_pool_info.h>
#include <order/simulator_order.h>

#include <json/json.h>
#include <json/json_object.h>
#include <json/json_value.h>
#include <cache/share_string.h>
#include <coroutine/event_base.h>
#include <order_book/order_book_snapshot.h>
#include <system_io/timer_io.h>
#include <system_io/https_server_io/https_socket_connection.h>
#include <system_io/https_websocket_server_io/https_websocket_connection_io.h>

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
        {"Order Book Snapshot Pool Size", OrderBookSnapShotPool::size()},
        {"Share String Pool Size", StringPool::size()},
        {"Task Info Event Pool Size", EpollBase::TaskInfoEventPool::size()},
        {"Timer IO Pool Size", TimerIOPool::size()},
        {"Https Client Socket Connection Pool Size", HttpsSocketConnectionPool::size()},
        {"Https Websocket Connection IO Pool Size", HttpsWebsocketConnectionIOPool::size()}
    };
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);;
}