#include <iomanip>

#include <api_handler/api_handler_binance_blvt/api_handler_binance_blvt_redeem.h>
#include <utils.h>
#include <user_manager/user_manager.h>
#include <storage_source/storage_source.h>
#include <app_utils.h>
#include <binance_utils.h>

APIHandlerBinanceBLVTRedeem::APIHandlerBinanceBLVTRedeem(HttpRequest* request) : APIHandlerBinanceBLVT(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

Json APIHandlerBinanceBLVTRedeem::handle_internal_request(Json& query_json)
{
    // thread safe
    static std::mutex internal_request_mutex;
    std::unique_lock lock(internal_request_mutex);

    const std::string user_id = "root";
    APIHandlerBinanceBLVTRedeem api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    SourceType type = StorageSource::get_source_type_by_name(BINANCE_SPOT_DB_SOURCE_NAME);
    api_handle.m_user->set_active_storage_source(type);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.handle_binance_request(query_json);
}

HttpResponse APIHandlerBinanceBLVTRedeem::child_handle()
{
    Json query_json = m_request->get_query_json();

    const std::string missing_param = m_request->check_missing_params({"symbol", "quantity"});
    if (missing_param == PARAM_NO_MISSING)
    {
        Json response = handle_binance_request(query_json);
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

void APIHandlerBinanceBLVTRedeem::handle_query_json(Json& query_json)
{
    // Remove field 'token'
    query_json.remove_field("token");
    query_json["symbol"].set_is_string_format(false);
}

Json APIHandlerBinanceBLVTRedeem::send_new_request(Json& query_json)
{
    //this->handle_query_json(query_json);
    std::string query_str = this->get_query_string(query_json);

    return send_binance_request("/sapi/v1/blvt/redeem", query_str, RequestMethod::POST);
}

std::string APIHandlerBinanceBLVTRedeem::get_query_string(Json& query_json)
{
    Json query;
    if (query_json.has_field("symbol"))
    {
        query["tokenName"] = query_json["symbol"];
    }
    if (query_json.has_field("quantity"))
    {
        query["amount"] = query_json["quantity"];
    }

    return m_request->get_query_string_from_query_json(query);
}

Json APIHandlerBinanceBLVTRedeem::handle_binance_request(Json& query_json)
{
    Json response;
    Json rest_response = send_new_request(query_json);
    ADD_LOG("order_data = " << rest_response.get_string_value());

    if (rest_response.has_field("code") && ((long)rest_response["code"]) < -1)
    {
        response["data"] = "";
        response["msg"] = rest_response["msg"];
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
        return response;
    }
    else
    {
        // Response to client
        this->standardize_response(rest_response);
        response["data"] = rest_response;
        response["msg"] = "Redeem BLVT token";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    return response;
}

void APIHandlerBinanceBLVTRedeem::standardize_response(Json& response)
{
    if (response.has_field("tokenName"))
    {
        response["symbol"] = response["tokenName"];
    }
    if (response.has_field("redeemAmount"))
    {
        response["quantity"] = response["redeemAmount"];
    }
    if (response.has_field("id"))
    {
        response["orderId"] = response["id"];
    }
    response["exchange"] = BINANCE_NAV_ABBREVIATION_NAME;
    response["side"] = "SELL";

    response.remove_field("id");
    response.remove_field("redeemAmount");
    response.remove_field("tokenName");

    ADD_LOG("standardize_response: " << response.get_string_value());
}