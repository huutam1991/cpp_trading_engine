#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <queue>

#include <coroutine/epoll_base.h>
#include <coroutine/task.h>
#include <coroutine/future.h>
#include <time/timer.h>
#include <network/tls_wrapper/tls_wrapper.h>
#include <system_io/https_client_request_io/https_client_request_io.h>

#include "https_client_request_builder.h"
#include "https_client_response_parser.h"
#include <network/tcp_connection/tcp_connection.h>

class HttpsClientRequest
{
    EpollBase* m_epoll_base = nullptr;
    std::string m_hostname;
    int m_port;
    std::vector<std::string> m_custom_headers;
    std::unique_ptr<TCPConnection> m_tcp_connection = nullptr;

public:
    HttpsClientRequest(EpollBase* epoll_base, const std::string& hostname, int port);
    ~HttpsClientRequest();

    void add_header(const std::string& key, const std::string& value);

    Task<HttpsClientResponse> get(const std::string& path);
    Task<HttpsClientResponse> post(const std::string& path, const std::string& body);
    Task<HttpsClientResponse> del(const std::string& path);
    Task<HttpsClientResponse> put(const std::string& path, const std::string& body);

protected:
    void on_disconnect();

private:
    HttpsClientResponseParser m_response_parser;
    std::queue<Future<HttpsClientResponse>::FutureValue*> m_response_futures;

    Task<void> wait_for_tcp_data();
    Task<HttpsClientResponse> send_request(const std::string& method, const std::string& path, const std::string& body = "");
};