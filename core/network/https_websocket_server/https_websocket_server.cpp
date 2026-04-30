#include "https_websocket_server.h"


HttpsWebsocketServer::HttpsWebsocketServer(int port, EpollBase* epoll_base)
    : m_https_websocket_server_io(std::make_unique<HttpsWebsocketServerIO>(port))
{
    m_https_websocket_server_io->set_callbacks(
        // on_connect callback
        [this](int fd, std::string path) -> Task<void>
        {
            spdlog::info("New websocket connection, fd = {}, path = {}", fd, path);

            WebsocketRouteName route = HttpsWebsocketServerRoute::get_route_from_path(path);
            m_websocket_routes_by_fd[fd] = m_routes[static_cast<int>(route)].get();

            HttpsWebsocketServerRoute* route_ptr = m_websocket_routes_by_fd[fd];
            if (route_ptr == nullptr)
            {
                spdlog::warn("HttpsWebsocketServer - no route found for path: [{}], cannot handle connection", path);
                co_return;
            }

            co_await route_ptr->on_connect(fd);

            co_return;
        },
        // on_message callback
        [this](int fd, std::string message) -> Task<void>
        {
            HttpsWebsocketServerRoute* route_ptr = m_websocket_routes_by_fd[fd];
            if (route_ptr == nullptr)
            {
                spdlog::warn("HttpsWebsocketServer - no route found for fd: {}, cannot handle on_message", fd);
                co_return;
            }

            co_await route_ptr->on_message(fd, message);

            co_return;
        },
        // on_disconnect callback
        [this](int fd) -> Task<void>
        {
            HttpsWebsocketServerRoute* route_ptr = m_websocket_routes_by_fd[fd];
            if (route_ptr == nullptr)
            {
                spdlog::warn("HttpsWebsocketServer - no route found for fd: {}, cannot handle disconnect", fd);
                co_return;
            }

            co_await route_ptr->on_disconnect(fd);
            m_websocket_routes_by_fd[fd] = nullptr;

            co_return;
        }
    );

    epoll_base->start_living_system_io_object(m_https_websocket_server_io.get());
}

void HttpsWebsocketServer::add_route(std::unique_ptr<HttpsWebsocketServerRoute> route)
{
    route->set_server(this);
    m_routes[static_cast<int>(route->get_route_enum())] = std::move(route);
}

void HttpsWebsocketServer::write_to_connection(int fd, std::string message)
{
    m_https_websocket_server_io->write_to_connection(fd, message);
}