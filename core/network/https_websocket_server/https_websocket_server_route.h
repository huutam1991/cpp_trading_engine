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

class HttpsWebsocketServer;

class HttpsWebsocketServerRoute
{
    WebsocketRouteName m_route_enum = WebsocketRouteName::none;
    std::string m_route_name = "none";
    HttpsWebsocketServer* m_server = nullptr;

public:
    HttpsWebsocketServerRoute(WebsocketRouteName route_enum = WebsocketRouteName::none)
        : m_route_enum(route_enum), m_route_name(enum_reflect::enum_name(route_enum))
    {}

    WebsocketRouteName get_route_enum() const { return m_route_enum; }
    std::string get_route_name() const { return m_route_name; }
    void set_server(HttpsWebsocketServer* server) { m_server = server; }

    virtual Task<void> on_connect(int fd);
    virtual Task<void> on_message(int fd, std::string message);
    virtual Task<void> on_disconnect(int fd);

    static WebsocketRouteName get_route_from_path(const std::string& path);
};