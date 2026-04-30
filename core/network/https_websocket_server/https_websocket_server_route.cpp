#include <json/json.h>

#include "https_websocket_server_route.h"
#include "https_websocket_server.h"

WebsocketRouteName HttpsWebsocketServerRoute::get_route_from_path(const std::string& path)
{
    return enum_reflect::enum_value<WebsocketRouteName>(path);
}

Task<void> HttpsWebsocketServerRoute::on_connect(int fd)
{
    spdlog::info("Websocket connection (fd = {}) connected to route [{}]", fd, enum_reflect::enum_name(m_route_enum));
    co_return;
}

Task<void> HttpsWebsocketServerRoute::on_message(int fd, std::string message)
{
    spdlog::info("Received message from websocket connection (fd = {}) on route [{}]: {}", fd, enum_reflect::enum_name(m_route_enum), message);

    Json response = {
        {"status", "ok"},
        {"route", m_route_name},
        {"message", "Route [none] is default route, no specific logic for this route. Please implement your own route by inheriting HttpsWebsocketServerRoute and override on_message function."}
    };

    m_server->write_to_connection(fd, response);

    co_return;
}

Task<void> HttpsWebsocketServerRoute::on_disconnect(int fd)
{
    spdlog::info("Websocket connection (fd = {}) disconnected from route [{}]", fd, enum_reflect::enum_name(m_route_enum));
    co_return;
}