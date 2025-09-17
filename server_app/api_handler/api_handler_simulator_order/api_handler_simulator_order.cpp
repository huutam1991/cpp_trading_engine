#include <api_handler/api_handler_simulator_order/api_handler_simulator_order.h>
#include <order/simulator_order.h>

#include <json/json.h>

APIHandlerSimulatorOrder::APIHandlerSimulatorOrder(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerSimulatorOrder::child_handle()
{
    Json response;
    std::string strategy_name = m_request->get_query_param("strategy_name");

    // GET
    if (m_request->get_request_method() == RequestMethod::GET)
    {
        response["data"] = SimulatorOrder::get_info();
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;

    }
    // POST
    else
    {
        Json config_data = m_request->get_body_json();
        bool active = config_data.has_field("active") ? (bool)config_data["active"] : false;
        SimulatorOrder::set_active(active);

        response["data"] = "";
        response["msg"] = "update simulator order config successfully";
        response["status_code"] = OK_200;
        response["error"] = false;
    }

    co_return HttpResponse(OK_200, response);;
}