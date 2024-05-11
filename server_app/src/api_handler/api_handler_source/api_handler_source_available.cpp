#include <api_handler/api_handler_source/api_handler_source_available.h>
#include <storage_source/storage_source.h>

APIHandlerSourceAvailable::APIHandlerSourceAvailable(HttpRequest* request) : APIHandler(request)
{}

HttpResponse APIHandlerSourceAvailable::child_handle()
{
    Json response;
    response["data"] = StorageSource::get_available_source();
    response["msg"] = "";
    response["status_code"] = OK_200;
    response["error"] = false;

    return HttpResponse(OK_200, response);
}