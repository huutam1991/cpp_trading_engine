#include <api_handler/api_handler_order/api_handler_order_list.h>
#include <order/order_manager.h>
#include <spdlog/spdlog.h>

APIHandlerOrderList::APIHandlerOrderList(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerOrderList::child_handle()
{
    Json response;

    // Get all orders
    std::vector<Order> orders = OrderManager::instance().get_all_orders();

    Json orders_json;
    for (auto& order : orders)
    {
        orders_json.push_back(order.to_json());
    }

    // Response
    response["orders"] = orders_json;
    response["msg"] = "Get order list successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);
}
