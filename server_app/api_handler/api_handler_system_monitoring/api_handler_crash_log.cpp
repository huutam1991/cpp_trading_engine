#include <json/json.h>
#include <mongo_db/mongo_db.h>
#include <utils/utils.h>

#include <api_handler/api_handler_system_monitoring/api_handler_crash_log.h>
#include <order/simulator_order.h>


APIHandlerCrashLog::APIHandlerCrashLog(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
}

Task<HttpResponse> APIHandlerCrashLog::child_handle()
{
    Json crash_logs = MongoDB::instance()
        .set_db_and_collection("system_monitoring", "crash_log")
        .find_many();

    crash_logs.sort([](Json& a, Json& b)
    {
        size_t timestamp_a = a["created_at_ns"];
        size_t timestamp_b = b["created_at_ns"];
        return timestamp_a > timestamp_b; // Sort in descending order
    });

    crash_logs.for_each([](Json& crash_log)
    {
        crash_log.remove_field("_id"); // Remove MongoDB internal ID field

        // Convert created_at_ns to human-readable format and add it as created_at field
        size_t created_at_ns = crash_log["created_at_ns"];
        crash_log["created_at"] = Utils::get_string_time_from_utc_nanoseconds(created_at_ns);
    });

    Json response;
    response["msg"] = "Find crash logs successfully";
    response["data"] = crash_logs;
    response["status_code"] = OK_200;
    response["error"] = false;

    co_return HttpResponse(OK_200, response);;
}