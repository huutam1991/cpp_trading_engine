#include "https_client_request.h"
#include "https_client_request_builder.h"
#include "https_client_response_parser.h"

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

    get("/check_health");
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
    if (!response_parser.parse(resp))
    {
        spdlog::error("HttpsClientRequest::on_response_received - Failed to parse HTTP response");
        return;
    }

    if (resp.is_complete)
    {
        spdlog::info("HTTP Response status: {} {}", resp.status_code, resp.status_message);
        spdlog::info("HTTP Body: {}", resp.body);
    }
}

Task<void> HttpsClientRequest::re_connect()
{
    // Retry connection after 5 seconds
    co_await Timer::sleep_for(5000);
    connect();
}

void HttpsClientRequest::get(const std::string& path)
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
}

void HttpsClientRequest::send(const std::string& request)
{

}