#include <functional>
#include <string>

#include <network/https_server/route/route.h>

Route::Route(RequestMethod method)
    : m_method(method)
{
}

Route::Route(RequestMethod method, RequestHandleFunction handle_function)
    : m_method(method), m_handle_function(handle_function)
{
}

RequestHandleFunction& Route::get_handle_function()
{
    return m_handle_function;
}

void Route::operator+(RequestHandleFunction handle_function)
{
    m_handle_function = handle_function;
}
