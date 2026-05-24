#include <utils/constants.h>
#include <network/https_server/route/route_controller.h>
#include <network/https_server/request/http_request.h>

void add_bad_request()
{
    ADD_CUSTOM_BAD_REQUEST
    {
        if (request == nullptr)
        {
            return HttpResponse(NOT_FOUND_404, NOT_FOUND_ERROR_MESSAGE);
        }

        return request->send_file_from_directory("templates/bad_request_404.html");
    });
}