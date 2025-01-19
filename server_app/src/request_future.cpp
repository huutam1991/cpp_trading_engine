#include <request_future.h>

RequestFuture::RequestFuture(const std::string& url, const std::string& port, const std::string& path, RequestMethod request_method)
    : m_request{std::make_shared<ExternalRequestSsl>(url, port, path, request_method)}
{}

void RequestFuture::add_header(const std::string& key, const std::string value)
{
    m_request->add_header(key, value);
}

void RequestFuture::add_body(Json& body)
{
    m_request->add_body(body);
}

Future<Json> RequestFuture::send_request()
{
    Future<Json> future([request = m_request](Future<Json>::FutureValue value) mutable
    {
        AppUtils::instance().get_app_pool()->execute_function([request = request, value]() mutable
        {
            Json response = Json::parse(request->send_request());
            ADD_LOG("response: "<< response);
            value.set_value(response);
        });
    });

    return future;
}