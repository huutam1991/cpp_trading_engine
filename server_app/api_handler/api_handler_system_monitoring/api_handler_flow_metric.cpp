#include <json/json.h>
#include <mongo_db/mongo_db.h>
#include <metric/flow_tracing.h>

#include <api_handler/api_handler_system_monitoring/api_handler_flow_metric.h>

APIHandlerFlowMetric::APIHandlerFlowMetric(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerFlowMetric::child_handle()
{
    Json response;
    response["msg"] = "Find crash logs successfully";
    response["data"] = FlowTracing::get_json_data();
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);;
}