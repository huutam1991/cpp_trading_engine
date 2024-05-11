#ifndef ROUTE_CONTROLLER_H
#define ROUTE_CONTROLLER_H

#include <string>
#include <unordered_map>

#include "route.h"
#include <util_macros.h>
#include <exception.h>

class RouteController
{
    Singleton(RouteController)

private:
    std::unordered_map<std::string, std::unordered_map<RequestMethod, Route*>> route_group_map;
    std::unordered_map<std::string, std::unordered_map<RequestMethod, Route*>> route_map;

    std::string check_handle_by_route_group(HttpRequest* request);
    std::string check_handle_by_route(HttpRequest* request);
    std::string check_send_file_from_dashboard_folder(HttpRequest* request);

    std::string m_dashboard_folder = "";

public:
    Route& add_route_group(RequestMethod method, const std::string& route_path);
    Route& add_route(RequestMethod method, const std::string& route_path);
    void   add_dashboard_folder(const std::string& dashboard_folder);
    std::string handle_request_base_on_route(HttpRequest* request);
};

#endif //ROUTE_CONTROLLER_H
