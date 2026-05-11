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
    order,
    market_maker,
    mean_reversion,
};

class HttpsWebsocketServer;

class HttpsWebsocketServerRoute
{
protected:
    WebsocketRouteName m_route_enum = WebsocketRouteName::none;
    std::string m_route_name = "none";
    HttpsWebsocketServer* m_server = nullptr;
    bool m_need_check_authentication = false;

public:
    HttpsWebsocketServerRoute(WebsocketRouteName route_enum = WebsocketRouteName::none, bool need_check_authentication = false)
        : m_route_enum(route_enum), m_route_name(enum_reflect::enum_name(route_enum)), m_need_check_authentication(need_check_authentication)
    {}

    WebsocketRouteName get_route_enum() const { return m_route_enum; }
    std::string get_route_name() const { return m_route_name; }
    bool need_check_authentication() const { return m_need_check_authentication; }
    void set_server(HttpsWebsocketServer* server) { m_server = server; }

    virtual Task<void> on_connect(int fd);
    virtual Task<void> on_message(int fd, std::string message);
    virtual Task<void> on_disconnect(int fd);

    static WebsocketRouteName get_route_from_path(const std::string& path);
};