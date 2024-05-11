#include <app_utils.h>
#include <api_handler/api_handler_user/api_handler_user_active_source.h>

APIHandlerUserActiveSource::APIHandlerUserActiveSource(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"source", "enviroment"});
}

HttpResponse APIHandlerUserActiveSource::child_handle()
{
    std::string source         = m_request->get_body_param_string("source");
    std::string enviroment     = m_request->get_body_param_string("enviroment");
    std::string db_source_name = source + "_" + enviroment;
    std::string user_id        = m_user->get_user_id();

    ADD_LOG("APIHandlerUserActiveSource 1");

    // Check if source name is valid
    if (StorageSource::check_valid_source_name(db_source_name) == false)
    {
        Json response;
        response["data"] = "";
        response["msg"] = "Wrong source name or enviroment";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;

        return HttpResponse(BAD_REQUEST_400, response);
    }
    ADD_LOG("APIHandlerUserActiveSource 2");

    // Check if user has registered this source
    if (AppUtils::instance().check_is_source_registered_by_user(db_source_name, m_user.get()) == false)
    {
        Json response;
        response["data"] = "";
        response["msg"] = "User [" + m_user->get_user_id() + "] hasn't registered for: source [" + source + "] - enviroment [" + enviroment + "]";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;

        return HttpResponse(BAD_REQUEST_400, response);
    }
    ADD_LOG("APIHandlerUserActiveSource 3");

    // Re-init user storage source to get updated data
    SourceType old_source_type = m_user->get_active_storage_source()->get_source_type();
    ADD_LOG("APIHandlerUserActiveSource 4");
    SourceType type = StorageSource::get_source_type_by_name(db_source_name);
    ADD_LOG("APIHandlerUserActiveSource 5");
    std::string active_source_result = m_user->set_active_storage_source(type);
    ADD_LOG("APIHandlerUserActiveSource 6");

    if (active_source_result != INIT_STORAGE_SOURCE_SUCCESS)
    {
        // Set active source to old source type
        m_user->set_active_storage_source(old_source_type);

        Json response;
        response["data"] = "";
        response["msg"] = active_source_result;
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;

        return HttpResponse(BAD_REQUEST_400, response);
    }
    ADD_LOG("APIHandlerUserActiveSource 7");

    // Response success
    Json binance_info = MongoDB::instance()
        .set_db_and_collection(db_source_name, "info")
        .find_one("user_id", user_id);
    ADD_LOG("APIHandlerUserActiveSource 8");

    Json response;
    response["data"] = {
        {"api_secret", binance_info["api_secret"]},
        {"api_key", binance_info["api_key"]}
    };
    response["msg"] = "Active source [" + source + "] - enviroment [" + enviroment + "] for user [" + m_user->get_user_id() + "] successfully";
    response["status_code"] = OK_200;
    response["error"] = false;
    ADD_LOG("APIHandlerUserActiveSource 9");

    return HttpResponse(OK_200, response);
}