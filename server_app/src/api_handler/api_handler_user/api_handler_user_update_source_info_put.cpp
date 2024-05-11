#include <api_handler/api_handler_user/api_handler_user_update_source_info_put.h>

APIHandlerUserUpdateSourceInfoPut::APIHandlerUserUpdateSourceInfoPut(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"source", "enviroment"});
}

HttpResponse APIHandlerUserUpdateSourceInfoPut::child_handle()
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

    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(db_source_name, "info");

    // Check counter
    int count = query.count_documents("user_id", user_id);
    if (count == 0)
    {
        Json response;
        response["data"] = "";
        response["msg"] = "Source [" + source + "] - enviroment [" + enviroment + "] for user [" + user_id + "] does not exist";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;

        return HttpResponse(BAD_REQUEST_400, response);
    }

    // Save data
    Json save_data = query.find_one("user_id", user_id);
    save_data.remove_field("_id");

    // Update fields
    m_request->get_body_json().for_each_with_key([&db_source_name, &user_id, &query](const std::string& key, Json& value)
    {
        if (key != "source" && key != "enviroment")
        {
            query.update_one("user_id", user_id, key, (std::string&&)value);
        }
    });

    // Verify valid source
    std::shared_ptr<StorageSource> storage_source = StorageSource::generate_storage_souce_by_type(StorageSource::get_source_type_by_name(db_source_name));
    storage_source->set_user_id(m_user->get_user_id());
    std::string verify_storage_source_result = storage_source->verify_valid_source();

    if (verify_storage_source_result != INIT_STORAGE_SOURCE_SUCCESS)
    {
        // Revert to old data
        save_data.for_each_with_key([&query, &user_id](const std::string& key, Json& value)
        {
            query.update_one("user_id", user_id, key, (std::string&&)value);
        });

        Json response;
        response["data"] = "";
        response["msg"] = verify_storage_source_result;
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;

        return HttpResponse(BAD_REQUEST_400, response);
    }

    // Response success
    Json binance_info = query.find_one("user_id", user_id);
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