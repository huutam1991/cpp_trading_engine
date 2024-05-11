#include <api_handler/api_handler_binance_futures/api_handler_binance_futures_account_info.h>

APIHandlerBinanceFuturesAccountInfo::APIHandlerBinanceFuturesAccountInfo(HttpRequest* request) : APIHandlerBinanceFutures(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

Json APIHandlerBinanceFuturesAccountInfo::handle_internal_request(const std::string user_id)
{
    APIHandlerBinanceFuturesAccountInfo api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    SourceType type = StorageSource::get_source_type_by_name(BINANCE_FUTURES_DB_SOURCE_NAME);
    api_handle.m_user->set_active_storage_source(type);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.handle_binance_request();
}

HttpResponse APIHandlerBinanceFuturesAccountInfo::child_handle()
{
    Json response = handle_binance_request();
    return HttpResponse((ResponseStatusCode)response["status_code"], response);
}

Json APIHandlerBinanceFuturesAccountInfo::send_new_request()
{
    return send_binance_request("/fapi/v2/account", "", RequestMethod::GET);
}

Json APIHandlerBinanceFuturesAccountInfo::handle_binance_request()
{
    Json response;
    Json rest_response = send_new_request();
    // ADD_LOG("rest_response = " << rest_response.get_string_value());

    if (rest_response.has_field("code") && ((long)rest_response["code"]) < -1)
    {
        response["data"] = Json::create_array();
        response["msg"] = rest_response["msg"];
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
        return response;
    }
    else
    {
        Json std_response;
        this->standardize_response(rest_response, std_response);

        response["data"] = std_response;
        response["msg"] = "Get current account information";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    return response;
}

void APIHandlerBinanceFuturesAccountInfo::standardize_response(Json& response, Json& std_response)
{
    std_response["exchange"] = BINANCE_FUTURES_ABBREVIATION_NAME;
    if (response.has_field("updateTime"))
    {
        std_response["time"] = response["updateTime"];
    }

    if (response.has_field("positions"))
    {
        Json positions = response["positions"];
        Json position_list = Json::create_array();

        positions.for_each([&position_list](Json& position)
        {
            if (std::stold((std::string&&)position["positionAmt"]) != 0.0)
            {
                Json info;
                info["symbol"] = position["symbol"];
                info["price"] = position["entryPrice"];
                info["quantity"] = position["positionAmt"];
                position_list.push_back(info);
            }
        });

        std_response["positionList"] = position_list;
    }
    
    ADD_LOG("standardize_response: " << std_response.get_string_value());
}