#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <functional>

#include <coroutine/future.h>
#include <coroutine/epoll_base.h>
#include <network/tls_wrapper/tls_wrapper.h>
#include <system_io/https_client_request_io/https_client_request_io.h>

class HttpsClientRequest
{
    EpollBase* m_epoll_base = nullptr;
    std::unique_ptr<TlsWrapper> m_tls_wrapper = nullptr;
    std::string m_hostname;
    int m_port;
    std::string m_ip;
    HttpClientRequestIO m_io_object;

public:
    HttpsClientRequest(EpollBase* epoll_base, const std::string& hostname, int port);
    ~HttpsClientRequest();

private:
    static TlsContext* get_tls_context();
    std::string resolve_hostname();
};