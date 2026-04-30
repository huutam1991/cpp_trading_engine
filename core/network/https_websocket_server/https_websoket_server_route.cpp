#include "https_websoket_server_route.h"

WebsocketServerRouteEnum HttpsWebsocketServerRoute::get_route_from_path(const std::string& path)
{
    return enum_reflect::enum_value<WebsocketServerRouteEnum>(path);
}