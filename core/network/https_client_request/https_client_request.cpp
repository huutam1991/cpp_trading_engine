#include "https_client_request.h"

#include <netdb.h>
#include <arpa/inet.h>

HttpsClientRequest::HttpsClientRequest(EpollBase* epoll_base, const std::string& hostname, int port, std::unique_ptr<TCPConnection> tcp_connection)
    :   m_epoll_base{epoll_base},
        m_hostname{hostname},
        m_port{port},
        m_tcp_connection{tcp_connection ?
            std::move(tcp_connection) :
            std::make_unique<TCPConnection>(m_epoll_base, m_hostname, m_port, [this]() { this->on_connect(); }, [this]() { this->on_disconnect(); })
        }
{
    m_wait_for_tcp_data_task = wait_for_tcp_data();
    m_wait_for_tcp_data_task.start_running_on(m_epoll_base);
}

HttpsClientRequest::~HttpsClientRequest()
{
    spdlog::debug("HttpsClientRequest::~HttpsClientRequest - Destroying HttpsClientRequest to {}:{}", m_hostname, m_port);

    // Need to intendly destroy [m_wait_for_tcp_data_task]
    m_wait_for_tcp_data_task.destroy(true);
}

void HttpsClientRequest::on_connect()
{
    spdlog::debug("HttpsClientRequest::on_connect - Connected to {}:{}", m_hostname, m_port);
}

void HttpsClientRequest::on_disconnect()
{
    spdlog::error("HttpsClientRequest::on_disconnect - Disconnected from {}:{}", m_hostname, m_port);

    // Set error response for all pending futures
    while (m_response_futures.empty() == false)
    {
        m_response_futures.front()->set_value(HttpsClientResponse::create_error_response());
        m_response_futures.pop();
    }
}

Task<void> HttpsClientRequest::wait_for_tcp_data()
{
    while (true)
    {
        std::string data = co_await m_tcp_connection->wait_for_data();

        m_response_parser.append_data(std::move(data));
        std::vector<HttpsClientResponse> responses = m_response_parser.parse_all();

        for (auto& resp : responses)
        {
            if (m_response_futures.empty() == false)
            {
                m_response_futures.front()->set_value(std::move(resp));
                m_response_futures.pop();
            }
        }
    }

    co_return;
}

void HttpsClientRequest::add_header(const std::string& key, const std::string& value)
{
    m_custom_headers.push_back(key + ": " + value + "\r\n");
}

Task<HttpsClientResponse> HttpsClientRequest::get(const std::string& path)
{
    co_return co_await send_request("GET", path);
}

Task<HttpsClientResponse> HttpsClientRequest::post(const std::string& path, const std::string& body)
{
    co_return co_await send_request("POST", path, body);
}

Task<HttpsClientResponse> HttpsClientRequest::del(const std::string& path)
{
    co_return co_await send_request("DELETE", path);
}

Task<HttpsClientResponse> HttpsClientRequest::put(const std::string& path, const std::string& body)
{
    co_return co_await send_request("PUT", path, body);
}

Task<HttpsClientResponse> HttpsClientRequest::send_request(const std::string& method, const std::string& path, const std::string& body)
{
    // Check connection
    if (m_tcp_connection->is_disconnected() == true)
    {
        co_return HttpsClientResponse::create_error_response();
    }

    HttpsClientRequestBuilder request_builder;

    // Method <path> HTTP/1.1\r\n
    request_builder.append(method);
    request_builder.append(" ");
    request_builder.append(path);
    request_builder.append(" HTTP/1.1\r\n");

    // Host
    request_builder.append("Host: ");
    request_builder.append(m_hostname);
    request_builder.append("\r\n");

    // Headers
    request_builder.append("Connection: keep-alive\r\n");
    request_builder.append("User-Agent: Tam C++ Trading Engine\r\n");
    request_builder.append("Accept: */*\r\n");
    if (!body.empty())
    {
        std::string len_str = "Content-Length: " + std::to_string(body.size()) + "\r\n";
        request_builder.append(len_str);
        request_builder.append("Content-Type: application/json\r\n");
    }

    // Custom headers
    for (auto& header: m_custom_headers)
    {
        request_builder.append(header);
    }

    // End headers
    request_builder.append("\r\n");

    // Body content
    if (!body.empty())
    {
        request_builder.append(body);
    }

    // Send
    m_tcp_connection->write(request_builder.to_string());

    co_return co_await Future<HttpsClientResponse>([this](Future<HttpsClientResponse>::FutureValue* value) mutable
    {
        m_response_futures.push(value);
    });
}