#include <api_handler/api_handler_binance_spot/api_handler_binance_account_info.h>

APIHandlerBinanceAccountInfo::APIHandlerBinanceAccountInfo(HttpRequest* request) : APIHandlerBinance(request)
{
    m_need_check_authentication = true;
    m_need_check_none_source = true;
}

Json APIHandlerBinanceAccountInfo::handle_internal_request(const std::string user_id)
{
    APIHandlerBinanceAccountInfo api_handle(nullptr);

    // Get user's storage source
    api_handle.m_user = UserManager::instance().get_user_by_id(user_id);
    SourceType type = StorageSource::get_source_type_by_name(BINANCE_SPOT_DB_SOURCE_NAME);
    api_handle.m_user->set_active_storage_source(type);
    StorageSource* storage_source = api_handle.m_user->get_active_storage_source().get();

    // Create api handle + set authentication info
    api_handle.set_authen_info(storage_source);

    // Handle internal request
    return api_handle.handle_binance_request();
}

HttpResponse APIHandlerBinanceAccountInfo::child_handle()
{
    Json response = handle_binance_request();
    return HttpResponse((ResponseStatusCode)response["status_code"], response);
}

Json APIHandlerBinanceAccountInfo::send_new_request()
{
    return send_binance_request("/api/v3/account", "", RequestMethod::GET);
}

Json APIHandlerBinanceAccountInfo::handle_binance_request()
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

void APIHandlerBinanceAccountInfo::standardize_response(Json& response, Json& std_response)
{
    std_response["exchange"] = BINANCE_SPOT_ABBREVIATION_NAME;
    if (response.has_field("updateTime"))
    {
        std_response["time"] = response["updateTime"];
    }

    if (response.has_field("balances"))
    {
        Json balances = response["balances"];
        Json balance_list = Json::create_array();

        balances.for_each([&balance_list](Json& balance)
        {
            if (std::stold((std::string&&)balance["free"]) > 0)
            {
                Json info;
                info["symbol"] = balance["asset"];
                info["price"] = 0;
                info["quantity"] = balance["free"];
                balance_list.push_back(info);
            }
        });

        std_response["positionList"] = balance_list;
    }
    
    ADD_LOG("standardize_response: " << std_response.get_string_value());
}