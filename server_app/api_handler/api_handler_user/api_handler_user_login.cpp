#include <api_handler/api_handler_user/api_handler_user_login.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <app_utils/app_utils.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>

APIHandlerUserLogin::APIHandlerUserLogin(HttpRequest* request) : APIHandler(request)
{
    add_mandatory_body_params({"username", "password"});
}

Task<HttpResponse> APIHandlerUserLogin::child_handle()
{
    Json response;
    Json custom_header = nullptr;
    std::string username = m_request->get_body_param_string("username");
    std::string password = m_request->get_body_param_string("password");

    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned char upassword[100];
    std::copy(password.begin(), password.end(), upassword);

    SHA256(upassword, password.size(), hash);

    std::stringstream ss;

    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
    }
    std::string hashed_password = ss.str();

    Json user_account = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "user")
        .find_one("username", username);

    if (user_account == nullptr || (user_account["username"] != username || user_account["password"] != hashed_password))
    {
        response["data"] = "";
        response["msg"] = "Wrong username or password";
        response["status_code"] = BAD_REQUEST_400;
        response["error"] = true;
    }
    else
    {
        spdlog::info("Valid username + password: {}", username);

        // Return values
        std::string token = JWTManager::instance().generate_token({
            {"user_id", username},
            {"type", "user"}
        });

        // Response
        response["data"] = {
            {"token", token},
            {"user_id", username}
        };
        response["msg"] = "Login successfully";
        response["status_code"] = OK_200;
        response["error"] = false;

        custom_header["Set-Cookie"] = "accessToken=" + token + "; HttpOnly; Max-Age=86400;";
        custom_header["Set-Cookie"].set_is_string_format(false);

    }

    // Add token to header's Cookie
    HttpResponse res(OK_200, response);
    if (custom_header != nullptr)
    {
        res.add_custom_header(custom_header);
    }

    co_return res;
}
