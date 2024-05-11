#include <api_handler/api_handler_user/api_handler_user_update_source_info.h>

APIHandlerUserUpdateSourceInfo::APIHandlerUserUpdateSourceInfo(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"source", "enviroment"});
}

HttpResponse APIHandlerUserUpdateSourceInfo::child_handle()
{
    std::string source         = m_request->get_body_param_string("source");
    std::string enviroment     = m_request->get_body_param_string("enviroment");
    std::string db_source_name = source + "_" + enviroment;
    std::string user_id        = m_user->get_user_id();

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

    // Create json update_data to save to DB
    Json update_data;
    update_data["user_id"] = user_id;
    m_request->get_body_json().for_each_with_key([&update_data](const std::string& key, Json& value)
    {
        if (key != "source" && key != "enviroment")
        {
            update_data[key] = (std::string&&) value;
        }
    });

    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(db_source_name, "info");

    // Update or add new info data
    int count = query.count_documents("user_id", user_id);
    if (count == 1)
    {
        Json response;
        response["data"] = "";
        response["msg"] = "Source [" + source + "] - enviroment [" + enviroment + "] for user [" + m_user->get_user_id() + "] has existed";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;

        return HttpResponse(BAD_REQUEST_400, response);
    }
    else
    {
        query.insert_one(update_data);
    }

    // Re-init user storage source to get updated data
    SourceType old_source_type = m_user->get_active_storage_source()->get_source_type();
    SourceType type = StorageSource::get_source_type_by_name(db_source_name);
    std::string active_source_result = m_user->set_active_storage_source(type);

    if (active_source_result != INIT_STORAGE_SOURCE_SUCCESS)
    {
        // Set active source to old source type
        m_user->set_active_storage_source(old_source_type);

        // Remove the updated source info as it's incorrect
        query.delete_one("user_id", user_id);

        Json response;
        response["data"] = "";
        response["msg"] = active_source_result;
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;

        return HttpResponse(BAD_REQUEST_400, response);
    }

    Json binance_info = MongoDB::instance()
        .set_db_and_collection(db_source_name, "info")
        .find_one("user_id", user_id);

    // Response success
    Json response;
    response["data"] = {
        {"api_secret", binance_info["api_secret"]},
        {"api_key", binance_info["api_key"]}
    };
    response["msg"] = "Update source [" + source + "] - enviroment [" + enviroment + "] for user [" + m_user->get_user_id() + "] successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}