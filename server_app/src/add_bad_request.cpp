#include <constants.h>
#include <https_server/route/route_controller.h>
#include <https_server/request/http_request.h>

void add_bad_request()
{
    ADD_CUSTOM_BAD_REQUEST
    {
        return request->send_file_from_directory("templates/bad_request_404.html");
    });
}