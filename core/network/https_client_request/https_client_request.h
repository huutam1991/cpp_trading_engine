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

class HttpsClientRequest
{
    EpollBase* m_epoll_base = nullptr;
    std::string m_hostname;
    int m_port;
    std::vector<std::string> m_custom_headers;
    std::unique_ptr<HttpClientRequestIO> m_io_object;

public:
    HttpsClientRequest(EpollBase* epoll_base, const std::string& hostname, int port);
    ~HttpsClientRequest();

    void add_header(const std::string& key, const std::string& value);
    void on_disconnect();
    void on_response_received(const char* buffer, std::uint32_t size);

    Task<HttpsClientResponse> get(const std::string& path);
    Task<HttpsClientResponse> post(const std::string& path, const std::string& body);
    Task<HttpsClientResponse> del(const std::string& path);
    Task<HttpsClientResponse> put(const std::string& path, const std::string& body);

private:
    HttpsClientResponseParser m_response_parser;
    std::queue<Future<HttpsClientResponse>::FutureValue*> m_response_futures;

    void connect();
    Task<void> re_connect();

    Task<HttpsClientResponse> send_request(const std::string& method, const std::string& path, const std::string& body = "");
};