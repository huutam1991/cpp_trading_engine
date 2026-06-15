#include <json/json.h>
#include <mongo_db/mongo_db.h>
#include <utils/utils.h>

#include <api_handler/api_handler_system_monitoring/api_handler_request_log.h>

APIHandlerRequestLog::APIHandlerRequestLog(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerRequestLog::child_handle()
{
    Json request_log = MongoDB::instance()
        .set_db_and_collection("system_monitoring", "request_log")
        .find_many();

    request_log.sort([](Json& a, Json& b)
    {
        size_t timestamp_a = a["created_at_ns"];
        size_t timestamp_b = b["created_at_ns"];
        return timestamp_a > timestamp_b; // Sort in descending order
    });

    request_log.for_each([](Json& request_log)
    {
        request_log.remove_field("_id"); // Remove MongoDB internal ID field
        request_log.remove_field("created_at_ns"); // Remove created_at_ns field
    });

    // Remove request logs if there are too many logs (keep only the latest 300 logs)
    if (request_log.size() > 300)
    {
        MongoDB::instance()
            .set_db_and_collection("system_monitoring", "request_log")
            .drop();
    }

    Json response;
    response["msg"] = "Find request logs successfully";
    response["data"] = request_log;
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);;
}