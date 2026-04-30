#include "https_websocket_server.h"


HttpsWebsocketServer::HttpsWebsocketServer(int port)
    : m_https_websocket_server_io(std::make_unique<HttpsWebsocketServerIO>(port))
{
    m_https_websocket_server_io->set_callbacks(
        // on_connect callback
        [this](int fd, std::string path) -> Task<void>
        {
            spdlog::info("New websocket connection, fd = {}, path = {}", fd, path);

            WebsocketRouteName route = HttpsWebsocketServerRoute::get_route_from_path(path);
            m_websocket_routes_by_fd[fd] = m_routes[static_cast<int>(route)].get();

            co_await m_websocket_routes_by_fd[fd]->on_connect(fd);

            co_return;
        },
        // on_message callback
        [this](int fd, std::string message) -> Task<void>
        {
            spdlog::info("Received message from websocket connection (fd = {}) on route {}: {}",
                fd,
                enum_reflect::enum_name(m_websocket_routes_by_fd[fd]->get_route_enum()), message
            );

            co_await m_websocket_routes_by_fd[fd]->on_message(fd, message);

            co_return;
        },
        // on_disconnect callback
        [this](int fd) -> Task<void>
        {
            spdlog::info("Websocket connection disconnected, fd = {}, route = {}",
                fd,
                enum_reflect::enum_name(m_websocket_routes_by_fd[fd]->get_route_enum())
            );

            co_await m_websocket_routes_by_fd[fd]->on_disconnect(fd);
            m_websocket_routes_by_fd[fd] = nullptr;

            co_return;
        }
    );
}

void HttpsWebsocketServer::add_route(std::unique_ptr<HttpsWebsocketServerRoute> route)
{
    m_routes[static_cast<int>(route->get_route_enum())] = std::move(route);
}