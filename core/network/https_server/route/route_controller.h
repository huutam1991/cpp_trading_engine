#pragma once

#include <string>
#include <unordered_map>

#include <utils/utils.h>
#include <network/https_server/route/route.h>
#include <network/https_server/exception.h>

class RouteController
{
    Singleton(RouteController)

private:
    std::unordered_map<std::string, std::unordered_map<RequestMethod, Route*>> route_group_map;
    std::unordered_map<std::string, std::unordered_map<RequestMethod, Route*>> route_map;

    Task<std::string> check_handle_by_route_group(HttpRequest* request);
    Task<std::string> check_handle_by_route(HttpRequest* request);
    std::string check_send_file_from_dashboard_folder(HttpRequest* request);

    std::string m_dashboard_folder = "";

public:
    Route& add_route_group(RequestMethod method, const std::string& route_path);
    Route& add_route(RequestMethod method, const std::string& route_path);
    void   add_dashboard_folder(const std::string& dashboard_folder);
    Task<std::string> handle_request_base_on_route(HttpRequest* request);
};

