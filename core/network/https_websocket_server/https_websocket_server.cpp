#include "https_websocket_server.h"
#include <jwt/jwt_manager.h>
#include <expected>

std::expected<void, std::string> validate_websocket_path(std::string path)
{
    if (path.empty() || path[0] != '/')
    {
        return std::unexpected("Invalid path: " + path);
    }

    return {};
}

HttpsWebsocketServer::HttpsWebsocketServer(int port, EpollBase* epoll_base)
    : m_https_websocket_server_io(std::make_unique<HttpsWebsocketServerIO>(port))
{
    m_https_websocket_server_io->set_callbacks(
        // on_connect callback
        [this](int fd, std::string path, std::string bearer_token) -> Task<std::expected<bool, std::string>>
        {
            WebsocketRouteName route = HttpsWebsocketServerRoute::get_route_from_path(path);
            m_websocket_routes_by_fd[fd] = m_routes[static_cast<int>(route)].get();
            HttpsWebsocketServerRoute* route_ptr = m_websocket_routes_by_fd[fd];

            if (route_ptr == nullptr || route == WebsocketRouteName::none)
            {
                spdlog::warn("HttpsWebsocketServer - no route found for path: [{}], cannot handle connection", path);
                co_return std::unexpected("No route found for path: " + path);
            }

            spdlog::info("New websocket connection, fd = {}, path = {}, bearer_token = {}", fd, path, bearer_token);

            if (route_ptr->need_check_authentication())
            {
                if (check_is_valid_token(bearer_token) == false)
                {
                    spdlog::warn("HttpsWebsocketServer - invalid token for path: [{}], rejecting connection", path);
                    co_return std::unexpected("Invalid token for path: " + path);
                }
            }

            co_await route_ptr->on_connect(fd);

            co_return true;
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

            spdlog::info("Websocket disconnected, fd = {}, route = [{}]", fd, route_ptr->get_route_name());

            co_await route_ptr->on_disconnect(fd);
            m_websocket_routes_by_fd[fd] = nullptr;

            co_return;
        }
    );

    epoll_base->start_living_system_io_object(m_https_websocket_server_io.get());
}

bool HttpsWebsocketServer::check_is_valid_token(const std::string& token)
{
    std::string check_valid_token = JWTManager::instance().verify_token(token);
    if (check_valid_token != VALID_TOKEN)
    {
        return false;
    }

    Json payload = JWTManager::instance().get_payload(token);
    std::string type = payload["type"];

    if (type != "user")
    {
        return false;
    }

    // Get User from user_id
    std::string user_id = payload["user_id"];
    // m_user = UserManager::instance().get_user_by_id(user_id);

    if (user_id != "root")
    {
        return false;
    }

    return true;
}

void HttpsWebsocketServer::add_route(std::unique_ptr<HttpsWebsocketServerRoute> route)
{
    spdlog::info("Adding websocket route [/{}]", route->get_route_name());

    route->set_server(this);
    m_routes[static_cast<int>(route->get_route_enum())] = std::move(route);
}

void HttpsWebsocketServer::write_to_connection(int fd, std::string message)
{
    m_https_websocket_server_io->write_to_connection(fd, std::move(message));
}