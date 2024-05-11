#include <api_handler/api_handler_user/api_handler_user_websocket_token.h>
#include <mongo_db/mongo_db.h>
#include <jwt/jwt_manager.h>
#include <app_utils.h>

APIHandlerUserWebsocketToken::APIHandlerUserWebsocketToken(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

HttpResponse APIHandlerUserWebsocketToken::child_handle()
{
    Json response;

    response["data"] = {
        {"websocket_token", JWTManager::instance().generate_token({"user_id", m_user->get_user_id()})},
    };
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}
