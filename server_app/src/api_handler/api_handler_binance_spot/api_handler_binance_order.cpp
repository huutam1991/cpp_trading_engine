#include <api_handler/api_handler_binance_spot/api_handler_binance_order.h>
#include <utils.h>
#include <app_utils.h>

APIHandlerBinanceOrder::APIHandlerBinanceOrder(HttpRequest* request) : APIHandlerBinance(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

std::string APIHandlerBinanceOrder::get_open_order_has_same_quantity_and_price()
{
    std::string res = "0";

    std::string query_str = "symbol=" + m_request->get_query_param("symbol");
    Json open_order = send_binance_request("/api/v3/openOrders", query_str);
    // ADD_LOG("open_order = " << open_order);

    if (open_order.size() > 0)
    {
        open_order.for_each([this, &res](Json& json)
        {
            std::string open_order_symbol = json["symbol"];
            std::string open_order_side = json["side"];
            std::string request_symbol = this->m_request->get_query_param("symbol");
            std::string request_side = this->m_request->get_query_param("side");

            double open_order_price = std::stod((std::string&&)json["price"]);
            double open_order_quantity = std::stod((std::string&&)json["origQty"]);
            double request_price = std::stod(this->m_request->get_query_param("price"));
            double request_quantity = std::stod(this->m_request->get_query_param("quantity"));

            if (open_order_symbol == request_symbol &&
                open_order_side == request_side &&
                open_order_price == request_price
            )
            {
                res = std::to_string((long)json["orderId"]);
                ADD_LOG("res = " << res);
            }
        });
    }

    return res;
}

Json APIHandlerBinanceOrder::get_open_order_has_same_price()
{
    Json res = JsonNull();

    std::string query_str = "symbol=" + m_request->get_query_param("symbol");
    Json open_order = send_binance_request("/api/v3/openOrders", query_str);
    // ADD_LOG("open_order = " << open_order);

    if (open_order.size() > 0)
    {
        open_order.for_each([this, &res](Json& json)
        {
            std::string open_order_symbol = json["symbol"];
            std::string open_order_side = json["side"];
            std::string request_symbol = this->m_request->get_query_param("symbol");
            std::string request_side = this->m_request->get_query_param("side");

            long double open_order_price = std::stod((std::string&&)json["price"]);
            long double open_order_quantity = std::stod((std::string&&)json["origQty"]);
            long double request_price = std::stod(this->m_request->get_query_param("price"));
            long double request_quantity = std::stod(this->m_request->get_query_param("quantity"));

            if (open_order_symbol == request_symbol &&
                open_order_side == request_side &&
                open_order_price == request_price
            )
            {
                res["orderId"] = std::to_string((long)json["orderId"]);
                res["open_order_quantity"] = open_order_quantity;
                res["request_quantity"] = request_quantity;

                // ADD_LOG("res = " << res);
            }
        });
    }

    return res;
}

Json APIHandlerBinanceOrder::send_new_order_request()
{
    // std::string duplicate_order_id = get_open_order_has_same_quantity_and_price();
    std::string duplicate_order_id = "0";

    // Remove field 'token'
    Json query_json = m_request->get_query_json();
    query_json.remove_field("token");

    Json duplicate_order = get_open_order_has_same_price();
    ADD_LOG("duplicate_order = " << duplicate_order);
    if (duplicate_order.is_null() == false)
    {
        duplicate_order_id = (std::string&&)duplicate_order["orderId"];
        // long double open_order_quantity = duplicate_order["open_order_quantity"];
        // long double request_quantity = duplicate_order["request_quantity"];

        // // New quantity = open_order_quantity + request_quantity
        // query_json["quantity"] = std::to_string(open_order_quantity + request_quantity);
    }

    std::string query_str = m_request->get_query_string_from_query_json(query_json) +
                            "&type=LIMIT&timeInForce=GTC" + // type=LIMIT + timeInForce=GTC is default
                            "&cancelReplaceMode=ALLOW_FAILURE&cancelOrderId=" + duplicate_order_id;

    return send_binance_request("/api/v3/order/cancelReplace", query_str, RequestMethod::POST);
}

HttpResponse APIHandlerBinanceOrder::child_handle()
{
    // /api/v3/order?symbol=BNBUSDT&side=SELL&type=LIMIT&timeInForce=GTC&quantity=12&price=140
    // &timestamp={{timestamp}}&signature={{signature}}

    Json response;
    const std::string missing_param = m_request->check_missing_params({"symbol", "side", "quantity", "price"});

    if (missing_param == PARAM_NO_MISSING)
    {
        Json order_data = send_new_order_request();
        ADD_LOG("order_data = " << order_data);

        if (order_data.has_field("code") && ((long)order_data["code"]) < -1)
        {
            if (order_data.has_field("data") && order_data["data"].has_field("newOrderResponse"))
            {
                Json newOrderResponse = order_data["data"]["newOrderResponse"];

                if (newOrderResponse.has_field("code") && ((long)newOrderResponse["code"]) < -1)
                {
                    return HttpRequest::response_bad_request_400(newOrderResponse["msg"]);
                }
                else
                {
                    // Response to client
                    response["data"] = newOrderResponse;
                    response["msg"] = "";
                    response["status_code"] = OK_200;
                    response["error"] = false;
                }
            }
            else
            {
                return HttpRequest::response_bad_request_400(order_data["msg"]);
            }
        }
        else
        {
            if (order_data.has_field("newOrderResponse"))
            {
                Json newOrderResponse = order_data["newOrderResponse"];

                // Response to client
                response["data"] = newOrderResponse;
                response["msg"] = "";
                response["status_code"] = OK_200;
                response["error"] = false;
            }
            else
            {
                // Response to client
                response["data"] = order_data;
                response["msg"] = "";
                response["status_code"] = OK_200;
                response["error"] = false;
            }
        }

        return HttpResponse(OK_200, response);
    }
    else
    {
        return HttpRequest::response_bad_request_400(missing_param);
    }
}