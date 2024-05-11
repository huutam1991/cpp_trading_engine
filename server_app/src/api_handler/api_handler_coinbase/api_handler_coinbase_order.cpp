#include <api_handler/api_handler_coinbase/api_handler_coinbase_order.h>
#include <utils.h>

APIHandlerCoinbaseOrder::APIHandlerCoinbaseOrder(HttpRequest* request) : APIHandlerCoinbase(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;

    add_mandatory_params({"symbol", "side", "quantity", "price"});
}

HttpResponse APIHandlerCoinbaseOrder::child_handle()
{
    std::string symbol = m_request->get_query_param("symbol");
    std::string side = m_request->get_query_param("side");
    std::string quantity = m_request->get_query_param("quantity");
    std::string price = m_request->get_query_param("price");

    Json order;
    order["type"] = "limit";
    order["time_in_force"] = "GTC";
    order["size"] = quantity;
    order["side"] = side;
    order["price"] = price;
    order["product_id"] = symbol;

    Json response = {
        {"data", send_coinbase_request("/orders", "", order.get_string_value(), RequestMethod::POST)},
        {"msg", ""},
        {"status_code", OK_200},
        {"error", false}
    };

    return HttpResponse(OK_200, response);
}