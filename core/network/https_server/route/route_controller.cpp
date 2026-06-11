#include <functional>
#include <utility>
#include <iostream>

#include <network/https_server/route/route_controller.h>

Route& RouteController::add_route_group(RequestMethod method, const std::string& route_path)
{
    spdlog::info("Implement API (Group) endpoint: {}", route_path);
    Route* route = new Route(method);

    std::unordered_map<RequestMethod, Route*>& route_set = route_group_map[route_path];
    route_set[method] = route;

    return *route;
}

Route& RouteController::add_route(RequestMethod method, const std::string& route_path)
{
    spdlog::info("Implement API endpoint: {}", route_path);
    Route* route = new Route(method);

    std::unordered_map<RequestMethod, Route*>& route_set = route_map[route_path];
    route_set[method] = route;

    return *route;
}

void RouteController::add_dashboard_folder(const std::string& dashboard_folder)
{
    m_dashboard_folder = dashboard_folder;
}

Task<HttpResponse> RouteController::check_handle_by_route_group(HttpRequest* request)
{
    // If the route group is valid, handle the request
    for (auto it = route_group_map.begin(); it != route_group_map.end(); it++)
    {
        if (request->get_url().find(it->first) != std::string::npos)
        {
            std::unordered_map<RequestMethod, Route*>& route_set = it->second;

            auto it_route_set = route_set.find(request->get_request_method());
            if (it_route_set != route_set.end())
            {
                Route* route = it_route_set->second;
                RequestHandleFunction& handle_function = route->get_handle_function();
                HttpResponse response = co_await handle_function(request);
                co_return response;
            }
        }
    }

    co_return HttpResponse(ResponseStatusCode::NOT_FOUND_404, Json{});
}

Task<HttpResponse> RouteController::check_handle_by_route(HttpRequest* request)
{
    // If the route is valid, handle the request
    auto it = route_map.find(request->get_url());
    if (it != route_map.end())
    {
        std::unordered_map<RequestMethod, Route*>& route_set = it->second;

        auto it_route_set = route_set.find(request->get_request_method());
        if (it_route_set != route_set.end())
        {
            Route* route = it_route_set->second;
            RequestHandleFunction& handle_function = route->get_handle_function();
            HttpResponse response = co_await handle_function(request);
            co_return response;
        }
    }

    co_return HttpResponse(ResponseStatusCode::NOT_FOUND_404, Json{});
}

HttpResponse RouteController::check_send_file_from_dashboard_folder(HttpRequest* request)
{
    if (m_dashboard_folder != "" && request != nullptr)
    {
        std::string file_path = m_dashboard_folder + request->get_url();
        if (request->check_is_file_path_exist(file_path))
        {
            return request->send_file_from_directory(file_path);
        }
    }

    return HttpResponse(ResponseStatusCode::NOT_FOUND_404, Json{}); // Return empty response if not found, so that it can continue to check other routes or return 404
}

Task<HttpResponse> RouteController::handle_request_base_on_route(HttpRequest* request)
{
    HttpResponse response;
    std::string response_str;

    try
    {
        // Check route group map first
        response = co_await check_handle_by_route_group(request);
        response_str = response.get_response_in_string();

        // If there is no matching, check the route map
        if (response_str == "")
        {
            response = co_await check_handle_by_route(request);
            response_str = response.get_response_in_string();
        }
    }
    catch(ApiException const& e)
    {
        // LOG(ERROR) << "Error: " << e.msg() << std::endl;
        co_return HttpRequest::response_bad_request_400(e.msg());
    }
    catch(std::exception const& e)
    {
        // LOG(ERROR) << "Error: " << e.what() << std::endl;
        co_return HttpRequest::response_internal_error_500();
    }

    if (response_str == "")
    {
        response = check_send_file_from_dashboard_folder(request);
        response_str = response.get_response_in_string();
    }

    // If no matching at all, return not found 404 page
    if (response_str == "")
    {
        response = request->response_not_found_404();
    }

    co_return response;
}