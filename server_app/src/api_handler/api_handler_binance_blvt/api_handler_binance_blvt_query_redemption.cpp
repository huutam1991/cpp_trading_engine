#include <iomanip>

#include <api_handler/api_handler_binance_blvt/api_handler_binance_blvt_query_redemption.h>
#include <utils.h>
#include <user_manager/user_manager.h>
#include <storage_source/storage_source.h>
#include <app_utils.h>
#include <binance_utils.h>

APIHandlerBinanceBLVTQueryRedemption::APIHandlerBinanceBLVTQueryRedemption(HttpRequest* request) : APIHandlerBinanceBLVT(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

Json APIHandlerBinanceBLVTQueryRedemption::handle_internal_request(Json& query_json)
{
    // thread safe
    static std::mutex internal_request_mutex;
    std::unique_lock lock(internal_request_mutex);

    const std::string user_id = "root";
    APIHandlerBinanceBLVTQueryRedemption api_handle(nullptr);

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

HttpResponse APIHandlerBinanceBLVTQueryRedemption::child_handle()
{
    Json query_json = m_request->get_query_json();

    const std::string missing_param = m_request->check_missing_params({"id"});
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

void APIHandlerBinanceBLVTQueryRedemption::handle_query_json(Json& query_json)
{
    // Remove field 'token'
    query_json.remove_field("token");
    query_json["symbol"].set_is_string_format(false);
}

Json APIHandlerBinanceBLVTQueryRedemption::send_new_request(Json& query_json)
{
    //this->handle_query_json(query_json);
    std::string query_str = this->get_query_string(query_json);

    return send_binance_request("/sapi/v1/blvt/redeem/record", query_str, RequestMethod::GET);
}

std::string APIHandlerBinanceBLVTQueryRedemption::get_query_string(Json& query_json)
{
    Json query;
    if (query_json.has_field("symbol"))
    {
        query["tokenName"] = query_json["symbol"];
    }
    if (query_json.has_field("orderId"))
    {
        query["id"] = query_json["orderId"];
    }

    return m_request->get_query_string_from_query_json(query);
}

Json APIHandlerBinanceBLVTQueryRedemption::handle_binance_request(Json& query_json)
{
    Json response;
    Json rest_response = send_new_request(query_json);

    ADD_LOG("rest_response = " << rest_response.get_string_value());

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
        Json res = rest_response[0];
        // Response to client
        this->standardize_response(res);
        response["data"] = res;
        response["msg"] = "Query Redemption Record";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    return response;
}

void APIHandlerBinanceBLVTQueryRedemption::standardize_response(Json& response)
{
    if (response.has_field("id"))
    {
        response["orderId"] = response["id"];
    }
    if (response.has_field("tokenName"))
    {
        response["symbol"] = response["tokenName"];
    }
    if (response.has_field("amount"))
    {
        response["quantity"] = response["amount"];
    }
    if (response.has_field("nav"))
    {
        response["price"] = response["nav"];
    }
    if (response.has_field("netProceed"))
    {
        response["amount"] = response["netProceed"];
    }
    response["exchange"] = BINANCE_NAV_ABBREVIATION_NAME;
    response["side"] = "SELL";

    response.remove_field("id");
    response.remove_field("nav");
    response.remove_field("tokenName");
    response.remove_field("netProceed");

    ADD_LOG("standardize_response: " << response.get_string_value());
}