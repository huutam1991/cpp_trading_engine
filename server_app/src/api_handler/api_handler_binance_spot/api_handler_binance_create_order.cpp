#include <iomanip>

#include <api_handler/api_handler_binance_spot/api_handler_binance_create_order.h>
#include <utils.h>
#include <user_manager/user_manager.h>
#include <storage_source/storage_source.h>
#include <app_utils.h>
#include <binance_utils.h>

APIHandlerBinanceCreateOrder::APIHandlerBinanceCreateOrder(HttpRequest* request) : APIHandlerBinance(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

void APIHandlerBinanceCreateOrder::handle_query_json(Json& query_json)
{
    // Remove field 'token'
    query_json.remove_field("token");

    // std::string quantity = query_json["quantity"];
    // std::string symbol = query_json["symbol"];

    // int lot_size = m_url == BINANCE_SPOT_URL ?
    //     BinanceUtils::instance().get_lot_size_by_symbol_real_net(symbol):
    //     BinanceUtils::instance().get_lot_size_by_symbol_test_net(symbol);

    // // Format price
    // size_t round_up_price = m_url == BINANCE_SPOT_URL ?
    //     BinanceUtils::instance().get_round_up_price_by_symbol_real_net(symbol):
    //     BinanceUtils::instance().get_round_up_price_by_symbol_test_net(symbol);

    // if (query_json.has_field("price"))
    // {
    //     long double price = query_json["price"];
    //     std::stringstream ss;
    //     ss << std::fixed << std::setprecision(round_up_price) << price;
    //     query_json["price"] = ss.str();
    // }

    // if (query_json.has_field("stopPrice"))
    // {
    //     long double stop_price = query_json["stopPrice"];
    //     std::stringstream ss;
    //     ss << std::fixed << std::setprecision(round_up_price) << stop_price;
    //     query_json["stopPrice"] = ss.str();
    // }

    // query_json["quantity"] = Utils::instance().round_string_number(quantity, lot_size);
    query_json["symbol"].set_is_string_format(false);
    query_json["side"].set_is_string_format(false);
}

Json APIHandlerBinanceCreateOrder::send_new_order_request(Json& query_json)
{
    handle_query_json(query_json);

    // Check type - should be refactor later as APIHandlerBinanceOrderAllowCancel has the same code
    std::string type = "LIMIT";
    if (query_json.has_field("type"))
    {
        type = (std::string&&)query_json["type"];
        query_json.remove_field("type");
    }

    std::string query_str = m_request->get_query_string_from_query_json(query_json) +
           "&type=" + type +
           ((type == "LIMIT" || type == "STOP_LOSS_LIMIT") ? "&timeInForce=GTC" : ""); // timeInForce=GTC is only for LIMIT

    return send_binance_request("/api/v3/order", query_str, RequestMethod::POST);
}

Json APIHandlerBinanceCreateOrder::handle_send_order_request(Json& query_json)
{
    Json response;
    Json order_data = send_new_order_request(query_json);
    ADD_LOG("order_data = " << order_data);

    if (order_data.has_field("code") && ((long)order_data["code"]) < -1)
    {
        if (order_data.has_field("data") && order_data["data"].has_field("newOrderResponse"))
        {
            Json newOrderResponse = order_data["data"]["newOrderResponse"];

            if (newOrderResponse.has_field("code") && ((long)newOrderResponse["code"]) < -1)
            {
                response["data"] = "";
                response["msg"] = newOrderResponse["msg"];
                response["status_code"] = BAD_REQUEST_400;
                response["error"] = true;
                return response;
            }
            else
            {
                // Response to client
                response["data"] = newOrderResponse;
                response["msg"] = "Placed order Id = " + std::to_string((long)newOrderResponse["orderId"]);
                response["status_code"] = OK_200;
                response["error"] = false;
            }
        }
        else
        {
            response["data"] = "";
            response["msg"] = order_data["msg"];
            response["status_code"] = BAD_REQUEST_400;
            response["error"] = true;
            return response;
        }
    }
    else
    {
        if (order_data.has_field("newOrderResponse"))
        {
            Json newOrderResponse = order_data["newOrderResponse"];
            // Response to client
            response["data"] = newOrderResponse;
            response["msg"] = "Placed order Id = " + std::to_string((long)newOrderResponse["orderId"]);
            response["status_code"] = OK_200;
            response["error"] = false;
        }
        else
        {
            if (order_data.has_field("type"))
            {
                // return Json();
            }
            else
            {
                ADD_LOG("Response ACK: skip save to DB");
            }
            // Response to client
            response["data"] = order_data;
            response["msg"] = "Placed order Id = " + std::to_string((long)order_data["orderId"]);
            response["status_code"] = OK_200;
            response["error"] = false;
        }
    }

    return response;
}

Json APIHandlerBinanceCreateOrder::handle_internal_request(Json& query_json, const std::string user_id)
{
    // thread safe
    static std::mutex internal_request_mutex;
    std::unique_lock lock(internal_request_mutex);

    APIHandlerBinanceCreateOrder api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    SourceType type = StorageSource::get_source_type_by_name(BINANCE_SPOT_DB_SOURCE_NAME);
    api_handle.m_user->set_active_storage_source(type);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.handle_send_order_request(query_json);
}

HttpResponse APIHandlerBinanceCreateOrder::child_handle()
{
    // /api/v3/order?symbol=BNBUSDT&side=SELL&type=LIMIT&timeInForce=GTC&quantity=12&price=140
    // &timestamp={{timestamp}}&signature={{signature}}

    Json query_json = m_request->get_query_json();

    const std::string missing_param = m_request->check_missing_params({"symbol", "side", "quantity", "price"});
    if (missing_param == PARAM_NO_MISSING)
    {
        Json response = handle_send_order_request(query_json);
        if (response["error"] == true)
        {
            return HttpResponse(BAD_REQUEST_400, response);
        }
        else
        {
            return HttpResponse(OK_200, response);
        }
    }
    else
    {
        return HttpRequest::response_bad_request_400(missing_param);
    }
}