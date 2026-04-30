#include <json/json.h>

#include "https_websocket_server_route.h"
#include "https_websocket_server.h"

WebsocketRouteName HttpsWebsocketServerRoute::get_route_from_path(const std::string& path)
{
    std::string_view sv = path;

    // Remove leading '/' if exists
    if (!sv.empty() && sv.front() == '/')
    {
        sv.remove_prefix(1);
    }

    std::string_view result = sv;

    return enum_reflect::enum_value<WebsocketRouteName>(result);
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
        {"message", "Route [none] is default route, please check your path and make sure it's correct"}
    };

    if (m_server != nullptr)
    {
        m_server->write_to_connection(fd, response);
    }

    co_return;
}

Task<void> HttpsWebsocketServerRoute::on_disconnect(int fd)
{
    spdlog::info("Websocket connection (fd = {}) disconnected from route [{}]", fd, enum_reflect::enum_name(m_route_enum));
    co_return;
}