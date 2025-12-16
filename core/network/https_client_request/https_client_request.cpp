#include "https_client_request.h"

#include <netdb.h>
#include <arpa/inet.h>

HttpsClientRequest::HttpsClientRequest(EpollBase* epoll_base, const std::string& hostname, int port)
    :   m_epoll_base{epoll_base},
        m_hostname{hostname},
        m_port{port}
{
    // Connect
    connect();
}

HttpsClientRequest::~HttpsClientRequest()
{
    spdlog::info("HttpsClientRequest::~HttpsClientRequest - Destroying HttpsClientRequest to {}:{}", m_hostname, m_port);
}

void HttpsClientRequest::connect()
{
    m_io_object = std::make_unique<HttpClientRequestIO>(m_hostname, m_port);
    m_io_object->set_on_disconnect_callback([this]()
    {
        this->on_disconnect();
    });
    m_io_object->set_on_response_received_callback([this](const char* buffer, std::uint32_t size)
    {
        this->on_response_received(buffer, size);
    });
    m_epoll_base->start_living_system_io_object(m_io_object.get());
}

void HttpsClientRequest::on_disconnect()
{
    spdlog::error("HttpsClientRequest::on_disconnect - Disconnected from {}:{}", m_hostname, m_port);
    m_io_object = nullptr;

    re_connect().start_running_on(m_epoll_base);
}

void HttpsClientRequest::on_response_received(const char* buffer, std::uint32_t size)
{
    HttpsClientResponse resp;
    HttpsClientResponseParser response_parser;

    response_parser.append_data(buffer, size);
    std::vector<HttpsClientResponse> responses = response_parser.parse_all();

    spdlog::info("=================================================================");

    for (auto& resp : responses)
    {
        if (m_response_futures.empty() == false)
        {
            m_response_futures.front()->set_value(std::move(resp));
            m_response_futures.pop();
        }
    }

}

Task<void> HttpsClientRequest::re_connect()
{
    // Retry connection after 5 seconds
    co_await Timer::sleep_for(5000);
    connect();
}

Task<HttpsClientResponse> HttpsClientRequest::get(const std::string& path)
{
    RequestBuilder request_builder;

    // GET <path> HTTP/1.1
    request_builder.append("GET ");
    request_builder.append(path);
    request_builder.append(" HTTP/1.1\r\n");
    // Host: <hostname>
    request_builder.append("Host: ");
    request_builder.append(m_hostname);
    request_builder.append("\r\n");
    // Connection: keep-alive
    request_builder.append("Connection: keep-alive\r\n");
    // User-Agent: ...
    request_builder.append("User-Agent: C++ Trading Engine\r\n");
    // Accept */*
    request_builder.append("Accept: */*\r\n");
    // End
    request_builder.append("\r\n");

    // Send request
    m_io_object->write(request_builder.to_string());

    co_return co_await Future<HttpsClientResponse>([this](Future<HttpsClientResponse>::FutureValue* value) mutable
    {
        m_response_futures.push(value);
    });
}

Task<HttpsClientResponse> HttpsClientRequest::post(const std::string& path, const std::string& body)
{
    RequestBuilder request_builder;

    // POST <path> HTTP/1.1\r\n
    request_builder.append("POST ");
    request_builder.append(path);
    request_builder.append(" HTTP/1.1\r\n");

    // Host: <hostname>\r\n
    request_builder.append("Host: ");
    request_builder.append(m_hostname);
    request_builder.append("\r\n");

    // Default headers
    request_builder.append("Connection: keep-alive\r\n");
    request_builder.append("User-Agent: C++ Trading Engine\r\n");
    request_builder.append("Accept: */*\r\n");
    request_builder.append("Content-Type: application/json\r\n");

    // Content-Length
    {
        std::string len_str = "Content-Length: " + std::to_string(body.size()) + "\r\n";
        request_builder.append(len_str);
    }

    // End of headers
    request_builder.append("\r\n");

    // Body
    if (!body.empty())
    {
        request_builder.append(body);
    }

    // Send
    m_io_object->write(request_builder.to_string());

    co_return co_await Future<HttpsClientResponse>([this](Future<HttpsClientResponse>::FutureValue* value) mutable
    {
        m_response_futures.push(value);
    });
}

Task<HttpsClientResponse> HttpsClientRequest::del(const std::string& path)
{
    RequestBuilder request_builder;

    // DELETE <path> HTTP/1.1\r\n
    request_builder.append("DELETE ");
    request_builder.append(path);
    request_builder.append(" HTTP/1.1\r\n");

    // Host: <hostname>\r\n
    request_builder.append("Host: ");
    request_builder.append(m_hostname);
    request_builder.append("\r\n");

    // Standard headers
    request_builder.append("Connection: keep-alive\r\n");
    request_builder.append("User-Agent: C++ Trading Engine\r\n");
    request_builder.append("Accept: */*\r\n");

    // End headers
    request_builder.append("\r\n");

    // Send request
    m_io_object->write(request_builder.to_string());

    co_return co_await Future<HttpsClientResponse>([this](Future<HttpsClientResponse>::FutureValue* value) mutable
    {
        m_response_futures.push(value);
    });
}

Task<HttpsClientResponse> HttpsClientRequest::put(const std::string& path, const std::string& body)
{
    RequestBuilder request_builder;

    // PUT <path> HTTP/1.1\r\n
    request_builder.append("PUT ");
    request_builder.append(path);
    request_builder.append(" HTTP/1.1\r\n");

    // Host
    request_builder.append("Host: ");
    request_builder.append(m_hostname);
    request_builder.append("\r\n");

    // Headers
    request_builder.append("Connection: keep-alive\r\n");
    request_builder.append("User-Agent: C++ Trading Engine\r\n");
    request_builder.append("Accept: */*\r\n");
    request_builder.append("Content-Type: application/json\r\n");

    // Content-Length
    {
        std::string len_str = "Content-Length: " + std::to_string(body.size()) + "\r\n";
        request_builder.append(len_str);
    }

    // End headers
    request_builder.append("\r\n");

    // Body content
    if (!body.empty())
    {
        request_builder.append(body);
    }

    // Send
    m_io_object->write(request_builder.to_string());

    co_return co_await Future<HttpsClientResponse>([this](Future<HttpsClientResponse>::FutureValue* value) mutable
    {
        m_response_futures.push(value);
    });
}
