#pragma once

#include <string>
#include <functional>

#include <utils/constants.h>
#include <network/https_server/request/http_request.h>

class Route
{
private:

protected:
    const RequestMethod m_method;
    RequestHandleFunction m_handle_function;

public:
    Route(RequestMethod method);
    Route(RequestMethod method, RequestHandleFunction handle_function);

    void operator+(RequestHandleFunction handle_function);
    RequestHandleFunction& get_handle_function();
};
