#pragma once

#include <string>

#include <spdlog/spdlog.h>
#include <enum_reflect/enum_reflect.h>
#include <coroutine/task.h>

enum class WebsocketRouteName
{
    none,
    ws,
    orderbook,
    market_maker,
    mean_reversion,
};

class HttpsWebsocketServerRoute
{
    WebsocketRouteName m_route_enum = WebsocketRouteName::none;

public:
    HttpsWebsocketServerRoute(WebsocketRouteName route_enum = WebsocketRouteName::none) : m_route_enum(route_enum) {};

    WebsocketRouteName get_route_enum() const { return m_route_enum; }

    virtual Task<void> on_connect(int fd);
    virtual Task<void> on_message(int fd, std::string message);
    virtual Task<void> on_disconnect(int fd);

    static WebsocketRouteName get_route_from_path(const std::string& path);
};