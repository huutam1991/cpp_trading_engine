#include <api_handler/api_handler_system_monitoring/api_handler_object_pool_info.h>
#include <order/simulator_order.h>

#include <json/json.h>
#include <json/json_object.h>
#include <json/json_value.h>
#include <cache/share_string.h>
#include <coroutine/event_base.h>
#include <order_book/order_book_snapshot.h>
#include <order_book/order_book.h>
#include <system_io/timer_io.h>
#include <system_io/https_server_io/https_socket_connection.h>
#include <system_io/https_websocket_server_io/https_websocket_connection_io.h>

APIHandlerObjectPoolInfo::APIHandlerObjectPoolInfo(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerObjectPoolInfo::child_handle()
{
    static EpollBase* epoll_system_io_task = static_cast<EpollBase*>(
        EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_SYSTEM_IO_TASK)
    );
    static EpollBase* epoll_gateway = static_cast<EpollBase*>(
        EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY)
    );

    Json response;
    response["data"] = {
        {"Json Object Pool Size", JsonObjectPool::size()},
        {"Json Value Pool Size", JsonValuePool::size()},
        {"Order Book Snapshot Pool Size", OrderBookSnapShotPool::size()},
        {"Order Book Update Pool Size", OrderBookUpdatePool::size()},
        {"Share String Pool Size", StringPool::size()},
        {"Timer IO Pool Size", TimerIOPool::size()},
        {"Https Client Socket Connection Pool Size", HttpsSocketConnectionPool::size()},
        {"Https Websocket Connection IO Pool Size", HttpsWebsocketConnectionIOPool::size()},
        {"Epoll System IO Task Pool Size", epoll_system_io_task->size()},
        {"Epoll Gateway Pool Size", epoll_gateway->size()}
    };
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);;
}