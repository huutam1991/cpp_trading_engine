#ifndef ROUTE_H
#define ROUTE_H

#include <string>
#include <functional>

#include <constants.h>
#include <request/http_request.h>

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

#endif //ROUTE_H
