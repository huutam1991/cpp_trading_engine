#pragma once

#include <string>

#include <enum_reflect/enum_reflect.h>

enum class WebsocketServerRouteEnum
{
    none,
    ws,
    orderbook,
    market_maker,
    mean_reversion
};

class HttpsWebsocketServerRoute
{
public:
    static WebsocketServerRouteEnum get_route_from_path(const std::string& path);
};