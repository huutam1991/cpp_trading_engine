#include <api_handler/api_handler_user/api_handler_user_register.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <app_utils.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

APIHandlerUserRegister::APIHandlerUserRegister(HttpRequest* request) : APIHandler(request)
{
    add_mandatory_body_params({"username", "password"});
}

HttpResponse APIHandlerUserRegister::child_handle()
{
    Json response;
    std::string username = m_request->get_body_param_string("username");
    std::string password = m_request->get_body_param_string("password");

    Json user_account = MongoDB::instance()
        .set_db_and_collection(USER_DB_NAME, "account")
        .find_one("username", username);

    if (user_account.is_null() == false)
    {
        response["data"] = "";
        response["msg"] = "Username already exist";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }
    else
    {
        ADD_LOG("Registering");

        unsigned char hash[SHA256_DIGEST_LENGTH];
        unsigned char upassword[password.size()];
        std::copy(password.begin(), password.end(), upassword);

        SHA256(upassword, password.size(), hash);

        std::stringstream ss;

        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
        }
        std::string hashed_password = ss.str();

        user_account["username"] = username;
        user_account["password"] = hashed_password;

        MongoDB::instance()
            .set_db_and_collection(USER_DB_NAME, "account")
            .insert_one(user_account);

        // Response
        response["data"] = {
            {"user_id", username}
        };
        response["msg"] = "Register successfully";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    return HttpResponse(OK_200, response);;
}