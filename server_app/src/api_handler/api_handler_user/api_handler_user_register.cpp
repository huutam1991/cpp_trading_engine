#include <api_handler/api_handler_user/api_handler_user_register.h>
#include <user/user_manager.h>

APIHandlerUserRegister::APIHandlerUserRegister(HttpRequest* request) : APIHandler(request)
{
    add_mandatory_body_params({"username", "password"});
}

HttpResponse APIHandlerUserRegister::child_handle()
{
    Json response;
    std::string username = m_request->get_body_param_string("username");
    std::string password = m_request->get_body_param_string("password");

    bool is_register_success = UserManager::instance().register_new_user(username, password);

    if (is_register_success == false)
    {
        response["data"] = "";
        response["msg"] = "Username already exist";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }
    else
    {
        // Response
        response["data"] = {
            {"username", username}
        };
        response["msg"] = "Register successfully";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    return HttpResponse(OK_200, response);;
}