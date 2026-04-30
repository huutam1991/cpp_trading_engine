#include "https_websocket_server.h"


HttpsWebsocketServer::HttpsWebsocketServer(int port)
    : m_https_websocket_server_io(std::make_unique<HttpsWebsocketServerIO>(port))
{
    m_https_websocket_server_io->set_callbacks(
        // on_connect callback
        [](int fd, std::string path) -> Task<void>
        {
            spdlog::info("New websocket connection, fd = {}, path = {}", fd, path);
            co_return;
        },
        // on_message callback
        [](int fd, std::string message) -> Task<void>
        {
            spdlog::info("Received message from websocket connection (fd = {}): {}", fd, message);
            co_return;
        },
        // on_disconnect callback
        [](int fd) -> Task<void>
        {
            spdlog::info("Websocket connection disconnected, fd = {}", fd);
            co_return;
        }
    );
}

void HttpsWebsocketServer::add_route(WebsocketServerRouteEnum route_enum, std::unique_ptr<HttpsWebsocketServerRoute> route)
{
    m_routes[static_cast<int>(route_enum)] = std::move(route);
}