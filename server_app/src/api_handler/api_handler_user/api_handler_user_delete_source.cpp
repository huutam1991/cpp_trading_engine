#include <api_handler/api_handler_user/api_handler_user_delete_source.h>
#include <app_utils.h>

APIHandlerUserDeleteSource::APIHandlerUserDeleteSource(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_body_params({"source", "enviroment"});
}

HttpResponse APIHandlerUserDeleteSource::child_handle()
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

    // Get active source name
    std::string active_source_name = m_user->get_storage_source_db_name();

    // Delete source
    MongoDB::instance()
        .set_db_and_collection(db_source_name, "info")
        .delete_one("user_id", user_id);

    // Set active source to None if active_source_name and db_source_name are the same
    if (active_source_name == db_source_name)
    {
        m_user->set_active_storage_source(SourceType::NONE);
    }

    // Response success
    Json response;
    response["data"] = "";
    response["msg"] = "Delete source [" + source + "] - enviroment [" + enviroment + "] for user [" + m_user->get_user_id() + "] successfully";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}
